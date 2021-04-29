/**
 * Brief:
 **/

#include <stdint.h>
#include <stdbool.h>

#define TAG "INPUT-KEYS"

#include "utils/log.h"

#include "hal/hal_mcu.h"
#include "hal/hal_irq.h"

#include "utils/intrinsics.h"
#include "utils/miscdefs.h"

#include "os/osfunc.h"

#include "input.h"
#include "i2c_hci.h"

/* */
static uint16_t pAdcRawSamples[3] = {0};
static uint16_t pAdcSamples[3] = {0};

/* Internal keys status */
/*
static uint16_t nKeysPressed = 0;
static uint16_t nKeysReleased = 0;

#define ADC_KEY1    0x0001
#define ADC_KEY2    0x0002
#define ADC_KEY3    0x0004
#define ADC_KEY4    0x0008
#define ADC_KEY4    0x0010
#define ADC_KEY5    0x0020
#define ADC_KEY6    0x0040
#define ADC_KEY7    0x0080
#define ADC_KEY8    0x0100
#define ADC_KEY9    0x0200
#define ADC_KEY10   0x0400
#define ADC_KEY11   0x0800
#define ADC_KEY12   0x1000
#define ADC_KEY13   0x2000
#define ADC_KEY14   0x4000
#define ADC_KEY15   0x8000
*/

/* */
void InputKeys_Init(void)
{
    /* ADC_IN5, ADC_IN8, ADC_IN9, SYS_WKUP1, GPIOA1 */

    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    __delay_cycles(10);

    /* Ensure that ADC is off */
    if (ADC1->CR & ADC_CR_ADEN)
        ADC1->CR &= ~ADC_CR_ADEN;

    /* Enable ADC voltage regulator and wait */
    ADC1->CR &= ~ADC_CR_ADVREGEN;   /* Exit from deep power down */

    OS_DelayMs(10);  /* 10mS */

    /* Launch the calibration by setting ADCAL */
    ADC1->CR |= ADC_CR_ADCAL;
    while ((ADC1->CR & ADC_CR_ADCAL) != 0);

    /* Select PCLK by writing 11 in CKMODE */
    ADC1->CFGR2 |= (ADC_CFGR2_CKMODE_0 | ADC_CFGR2_CKMODE_1);
    /* Select the continuous mode and scanning direction */
    ADC1->CFGR1 |= ADC_CFGR1_WAIT |ADC_CFGR1_CONT | ADC_CFGR1_SCANDIR;
    /* */
    ADC1->CHSELR = ADC_CHSELR_CHSEL5 | ADC_CHSELR_CHSEL8 | ADC_CHSELR_CHSEL9;
    /* 160.5 ADC clock cycles */
    ADC1->SMPR |= ADC_SMPR_SMP_0 | ADC_SMPR_SMP_1 | ADC_SMPR_SMP_2;
    /* Wake-up the VREFINT */
    ADC->CCR |= ADC_CCR_VREFEN;

    /* Oversampler */
    ADC1->CFGR2 |= ADC_CFGR2_OVSR_1; /* Oversampling ratio: 8x */
    ADC1->CFGR2 |= ADC_CFGR2_OVSS_0 | ADC_CFGR2_OVSS_2; /* Oversampling shift: 5-bits */
    ADC1->CFGR2 |= ADC_CFGR2_OVSE;

    /* Enable DMA transfer on ADC and circular mode */
    ADC1->CFGR1 |= ADC_CFGR1_DMAEN | ADC_CFGR1_DMACFG;

    DMA1_Channel1->CPAR = (uint32_t)(&(ADC1->DR));
    DMA1_Channel1->CMAR = (uint32_t)(&pAdcRawSamples);
    DMA1_Channel1->CNDTR = ARRAY_SIZE(pAdcRawSamples); 
    DMA1_Channel1->CCR |= DMA_CCR_MINC | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_TCIE | DMA_CCR_CIRC;
    DMA1_Channel1->CCR |= DMA_CCR_EN;

    ADC1->CR |= ADC_CR_ADEN;
    while ((ADC1->ISR & ADC_ISR_ADRDY) == 0);

    ADC1->CR |= ADC_CR_ADSTART;
    while (ADC1->CR & ADC_CR_ADSTP);
}

/*
Measured values on intKeys:
0x280 eq    640
0x3c        60 - 10 / 2 = +-25
0x244 mute  580
0x5c        92 - 10 / 2 = +-41
0x1e8 band  488
0x59        89 - 10 / 2 = +-39
0x18f navi  399
0xa8        168 - 10 / 2 = +- 79
0x0e7 home  231

Minimal value of diviation +- 25 points
*/

static int ValitadeDiviation(const uint16_t first, const uint16_t second, const uint16_t div)
{
     return (second >= (first - div) && second <=  (first + div));
}

void InputKeys_Update(void)
{
    /* debounce values */
    for(unsigned i = 0; i < ARRAY_SIZE(pAdcRawSamples); i++)
    {
        /* read first value */
        uint16_t first = pAdcRawSamples[i];
        OS_DelayMs(10);
        /* read sustained value */
        uint16_t sustained = pAdcRawSamples[i];

        /* check for value in range */
        if(!ValitadeDiviation(first, sustained, 25))
            continue;

        if(ValitadeDiviation(sustained, pAdcSamples[i], 25))
            continue;

        /* sample is changed */
        pAdcSamples[i] = sustained;

        I2CHCI_SetAdcValue(sustained, i);
        MAIN_ReportAdcValue();
    }

    /*
    uint16_t extKeys1 = pAdcRawSamples[0];
    uint16_t extKeys2 = pAdcRawSamples[1];
    uint16_t intKeys = pAdcRawSamples[2];

    OS_DelayMs(10);

    uint16_t exitKeys1Debounce = pAdcRawSamples[0];
    uint16_t exitKeys2Debounce = pAdcRawSamples[1];
    uint16_t intKeysDebounce = pAdcRawSamples[2];

    uint16_t min = (intKeys - 25);
    uint16_t max = (intKeys + 25);

    if((intKeysDebounce >= min && intKeysDebounce <= max) == 0) {
        return;
    }
*/
/*
    if(intKeys >= (640 - 25) && intKeys <= (640 + 25)) {
        nKeysPressed |= ADC_KEY1;
    }
    else if(intKeys >= (580 - 25) && intKeys <= (580 + 25)) {
        nKeysPressed |= ADC_KEY2;
    }
    else if(intKeys >= (488 - 25) && intKeys <= (488 + 25)) {
        nKeysPressed |= ADC_KEY3;
    }
    else if(intKeys >= (399 - 25) && intKeys <= (399 + 25)) {
        nKeysPressed |= ADC_KEY4;
    }
    else if(intKeys >= (231 - 25) && intKeys <= (231 + 25)) {
        nKeysPressed |= ADC_KEY5;
    }

    MAIN_EqKeyPressed();
    MAIN_MuteKeyPressed();
    MAIN_BandKeyPressed();
    MAIN_NaviKeyPressed();
    MAIN_SrcKeyPressed();
    */
}
