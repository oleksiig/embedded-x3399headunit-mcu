/**
 * Brief:
 **/

#include <stdint.h>
#include <stdbool.h>

#define TAG "INPUT-ENCODERS"

#include "utils/log.h"

#include "hal/hal_mcu.h"
#include "hal/hal_irq.h"

#include "os/osfunc.h"

#include "input.h"

void InputEncoders_Init(void)
{
    RCC->APB2ENR |= (RCC_APB2ENR_TIM21EN | RCC_APB2ENR_TIM22EN);

    /* -------------------------------------------------------------- */
    TIM21->CCMR1 = TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_1;
    TIM21->CCER = 0;
    TIM21->DIER = TIM_DIER_UIE;

    TIM21->PSC = 0;
    TIM21->ARR = 1;
    TIM21->CNT = 0;

    /* Use Encoder mode 3 */
    TIM21->SMCR = TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1;

    /* -------------------------------------------------------------- */
    TIM22->CCMR1 = TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_1;
    TIM22->CCER = 0;
    TIM22->DIER = TIM_DIER_UIE;

    TIM22->PSC = 0;
    TIM22->ARR = 1;
    TIM22->CNT = 0;

    /* Use Encoder mode 3 */
    TIM22->SMCR = TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1;

    /* -------------------------------------------------------------- */
    OS_IRQ_Enable(TIM21_IRQn);
    OS_IRQ_Enable(TIM22_IRQn);

    TIM21->CR1 = TIM_CR1_CEN;
    TIM22->CR1 = TIM_CR1_CEN;
}

__attribute__((used, interrupt))
    void HAL_IRQ_TIM21_Handler(void)
{
    if (TIM21->CR1 & TIM_CR1_DIR) {
        MAIN_VolumeStepDown();
    } else {
        MAIN_VolumeStepUp();
    }

    TIM21->SR &= ~(TIM_SR_UIF);
}

__attribute__((used, interrupt))
    void HAL_IRQ_TIM22_Handler(void)
{
    if (TIM22->CR1 & TIM_CR1_DIR) {
        MAIN_TuneStepBack();
    } else {
        MAIN_TuneStepForward();
    }

    TIM22->SR &= ~(TIM_SR_UIF);
}
