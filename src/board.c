/**
 * Brief:
 **/

#include <stdint.h>

#include "hal/hal_mcu.h"
#include "hal/hal_clock.h"
#include "hal/hal_gpio.h"
#include "hal/hal_dma.h"

/* -------------------------------------------------------------------- */
const HAL_ClkDesc_t g_pClkList[] =
{
    //{ 4194000,  (CLK_TYPE_OSC_MSI | CLK_MAIN_OSC) },
    { 4000000,  (CLK_TYPE_OSC_INT | CLK_MAIN_OSC) },
    { 4000000,  (CLK_TYPE_AHB)                    },
    { 4000000,  (CLK_TYPE_APB1)                   },
    { 4000000,  (CLK_TYPE_APB2)                   },
    { 0, 0 } /* End of list */
};

/* -------------------------------------------------------------------- */
const HAL_GpioPin_t g_pPinList[] =
{
    /* LEDs */
    GPIO_CONFIG(GPIOA, GPIO_BIT10,  GPIO_AF_NONE,   GPIO_OUT_PUSH_PULL_H),   // LD1 (Red)
    GPIO_CONFIG(GPIOA, GPIO_BIT11,  GPIO_AF_NONE,   GPIO_OUT_PUSH_PULL_H),   // LD2 (Green)
    GPIO_CONFIG(GPIOA, GPIO_BIT12,  GPIO_AF_NONE,   GPIO_OUT_PUSH_PULL_H),   // LD3 (Yellow)

    /* I2C1 host control interface */
    GPIO_CONFIG(GPIOB, GPIO_BIT6,   GPIO_AF1,       GPIO_OUT_OPEN_DRAIN),  // I2C1_SCL
    GPIO_CONFIG(GPIOB, GPIO_BIT7,   GPIO_AF1,       GPIO_OUT_OPEN_DRAIN),  // I2C1_SDA
    GPIO_CONFIG(GPIOB, GPIO_BIT5,   GPIO_AF_NONE,   GPIO_OUT_PUSH_PULL),   // GPIO_OUT - MCU_IRQ

    /* ENC_VOL */
    GPIO_CONFIG(GPIOA, GPIO_BIT2,   GPIO_AF0,       GPIO_IN_PULL_UP),      // TIM21_CH1 - ENC_VOL_A
    GPIO_CONFIG(GPIOA, GPIO_BIT3,   GPIO_AF0,       GPIO_IN_PULL_UP),      // TIM21_CH2 - ENC_VOL_B

    /* ENC_TUNE */
    GPIO_CONFIG(GPIOA, GPIO_BIT6,  GPIO_AF5,        GPIO_IN_PULL_UP),      // TIM22_CH1 - ENC_TUNE_A
    GPIO_CONFIG(GPIOA, GPIO_BIT7,  GPIO_AF5,        GPIO_IN_PULL_UP),      // TIM22_CH2 - ENC_TUNE_B

    /* KEYS */
    GPIO_CONFIG(GPIOA, GPIO_BIT5,  GPIO_AF_NONE,    GPIO_IN_ANALOG),       // ADC_IN5 - KEYS
    GPIO_CONFIG(GPIOB, GPIO_BIT0,  GPIO_AF_NONE,    GPIO_IN_ANALOG),       // ADC_IN8 - WHEEL_KEY1
    GPIO_CONFIG(GPIOB, GPIO_BIT1,  GPIO_AF_NONE,    GPIO_IN_ANALOG),       // ADC_IN9 - WHEEL_KEY2

    GPIO_CONFIG(GPIOA, GPIO_BIT0,  GPIO_AF_NONE,    GPIO_IN_PULL_UP),      // GPIO_IN - KEY_POWER
    GPIO_CONFIG(GPIOA, GPIO_BIT1,  GPIO_AF_NONE,    GPIO_IN_PULL_UP),      // GPIO_IN - KEY_EJECT
    GPIO_CONFIG(GPIOA, GPIO_BIT8,  GPIO_AF_NONE,    GPIO_IN_PULL_UP),      // GPIO_IN - ACC_KEY

    /* */
    GPIO_CONFIG(GPIOA, GPIO_BIT9,  GPIO_AF_NONE,    GPIO_OUT_PUSH_PULL_H), // GPIO_OUT - POWER_EN
    GPIO_CONFIG(GPIOB, GPIO_BIT3,  GPIO_AF_NONE,    GPIO_IN_PULL_DOWN),    // GPIO_IN - 5V0_STATUS

    /* End of list */
    GPIO_CONFIG(NULL, 0, 0, 0)
};
