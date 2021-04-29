/**
 * Brief:
 **/

#include <stdint.h>

#define TAG "I2CHCI"

#include "utils/log.h"
#include "utils/intrinsics.h"

#include "hal/hal_mcu.h"
#include "hal/hal_gpio.h"

#include "os/osfunc.h"

#include "i2c_hci.h"

/* */
typedef
enum e_IOStage {
    e_IOStage_Reset = 0,
    e_IOStage_SetAddrRx = 2,  /* Write transfer, slave enters receiver mode */
    e_IOStage_TxData = 3,
    e_IOStage_RxData = 4
} IOStage_t;

static uint8_t      IoAddr = 0;
static uint8_t      IoBuf[8] = {0};
static IOStage_t    IoStage = e_IOStage_Reset;

/* */
void I2CHCI_Init(const uint8_t ownAddr)
{
    /* --------------------------------------------------------------------- */
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    RCC->APB1RSTR |= RCC_APB1ENR_I2C1EN;
    RCC->APB1RSTR &= ~RCC_APB1ENR_I2C1EN;

    __delay_cycles(100);

    /* Disable I2C and set default flags */
    I2C1->CR1 &= ~I2C_CR1_PE;
    I2C1->CR1 &= ~(I2C_CR1_ANFOFF |     /* Analog noise filter on */
                    I2C_CR1_ERRIE);     /* Error interrupt disable */ 
    //I2C1->CR1 |= I2C_CR1_NOSTRETCH;     /* Disable clock stretching */

    /* Disable own address 1 & 2 */
    I2C1->OAR1 &= ~(I2C_OAR1_OA1EN | I2C_OAR2_OA2EN);
    I2C1->OAR1 = I2C_OAR1_OA1EN | (ownAddr & I2C_OAR1_OA1_Msk); /* Set 7-bit own address 1 */

    /* 0 = 7-bit mode; 1 = 10-bit */
    I2C1->CR2 &= ~I2C_CR2_ADD10;

    /* Set timings for 8MHz from HSI */
    I2C1->TIMINGR = ((0x0 << I2C_TIMINGR_PRESC_Pos) & I2C_TIMINGR_PRESC_Msk) |
                    ((0x3 << I2C_TIMINGR_SCLDEL_Pos) & I2C_TIMINGR_SCLDEL_Msk) |
                    ((0x1 << I2C_TIMINGR_SDADEL_Pos) & I2C_TIMINGR_SDADEL_Msk) |
                    ((0x3 << I2C_TIMINGR_SCLH_Pos) & I2C_TIMINGR_SCLH_Msk) |
                    ((0x9 << I2C_TIMINGR_SCLL_Pos) & I2C_TIMINGR_SCLL_Msk);

    /* Enable NACK generation (slave mode) */
    I2C1->CR2 |= I2C_CR2_NACK;

    /* Load first data byte to transmitter */
    I2C1->TXDR = IoBuf[IoAddr];
    IoBuf[IoAddr] = 0; /* clear last transmitted byte */
    IoAddr++;

    /* Enable I2C */
    I2C1->CR1 |= I2C_CR1_PE | I2C_CR1_STOPIE | I2C_CR1_RXIE /*| I2C_CR1_TXDMAEN*/ |
                 I2C_CR1_ADDRIE | I2C_CR1_ERRIE | I2C_CR1_NACKIE;

    OS_IRQ_Enable(I2C1_IRQn);
}

__attribute__((used, interrupt))
    void HAL_IRQ_I2C1_Handler(void)
{
    uint32_t isr = I2C1->ISR;

    if(isr & (I2C_ISR_BERR | I2C_ISR_OVR | I2C_ISR_ARLO))
    {
        I2C1->ICR = I2C_ICR_BERRCF | I2C_ICR_OVRCF | I2C_ICR_ARLOCF;
        I2C1->CR1 &= ~(I2C_CR1_TXIE | I2C_CR1_RXIE);
        I2C1->CR2 |= I2C_CR2_NACK;
        return;
    }

    if(isr & I2C_ISR_NACKF)
    {
        I2C1->ICR = I2C_ICR_NACKCF;
        I2C1->CR1 &= ~(I2C_CR1_TXIE | I2C_CR1_RXIE);
        I2C1->CR2 |= I2C_CR2_NACK;
        return;
    }

    if(isr & I2C_ISR_ADDR)
    {
        I2C1->ICR = I2C_ICR_ADDRCF;

        if(isr & I2C_ISR_DIR)
        {
            IoStage = e_IOStage_TxData;
            I2C1->CR1 |= I2C_CR1_TXIE;
            return;
        }
        else {
            IoStage = e_IOStage_SetAddrRx;
            I2C1->CR1 |= I2C_CR1_RXIE;
        }
    }

    if(isr & I2C_ISR_RXNE)
    {
        volatile uint8_t byte = I2C1->RXDR;

        if(IoStage == e_IOStage_SetAddrRx || IoStage == e_IOStage_RxData)
        {
            if(IoStage == e_IOStage_SetAddrRx)
            {
                IoAddr = byte;
                IoStage = e_IOStage_RxData;

                /* Load first data byte to transmitter due requirements */
                I2C1->TXDR = IoBuf[IoAddr];
                IoBuf[IoAddr] = 0; /* clear last transmitted byte */
                IoAddr++;
                return;
            }

            if(IoAddr >= sizeof(IoBuf))
            {
                I2C1->CR1 &= ~I2C_CR1_RXIE;
                I2C1->CR2 |= I2C_CR2_NACK;

                IoStage = e_IOStage_Reset;
            } else {
                IoBuf[IoAddr++] = byte;
            }

            return;
        }
    }

    if(IoStage == e_IOStage_TxData && (isr & I2C_ISR_TXIS))
    {
        if(IoAddr >= sizeof(IoBuf))
        {
            I2C1->CR1 &= ~I2C_CR1_TXIE;
            I2C1->CR2 |= I2C_CR2_NACK;

            IoStage = e_IOStage_Reset;
        }
        else {
            I2C1->TXDR = IoBuf[IoAddr];
            IoBuf[IoAddr] = 0; /* clear last transmitted byte */
            IoAddr++;
        }
    }

    if(isr & I2C_ISR_STOPF)
    {
        I2C1->ICR = I2C_ICR_STOPCF;

        I2C1->CR1 &= ~(I2C_CR1_TXIE | I2C_CR1_RXIE);
        I2C1->CR2 |= I2C_CR2_NACK;
    }
}

void I2CHCI_SetStatus(uint16_t status)
{
    /* 6, 7 */
    IoBuf[6] = (status >> 8) & 0xff;
    IoBuf[7] = (status) & 0xff;
}

void I2CHCI_SetAdcValue(uint16_t value, uint8_t group)
{
    if(group >= 3)
        return;

    /* 0, 1, 2, 3, 4, 5 */
    IoBuf[group]     = (value >> 8) & 0xff;
    IoBuf[group + 1] = (value) & 0xff;
}

/*
> key pressed -> update bits -> request host
*/