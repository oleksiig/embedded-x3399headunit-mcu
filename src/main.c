/**
 * Brief: 
 **/

#include <stdint.h>

#define TAG "Main"

#include "utils/log.h"
#include "utils/intrinsics.h"
#include "utils/miscdefs.h"
#include "hal/hal_mcu.h"
#include "hal/hal_gpio.h"

#include "os/osfunc.h"

#include "input.h"
#include "i2c_hci.h"

/* -------------------------------------------------------------------- */
typedef
enum PowerStage_e
{
    e_PowerOff = 0,
    e_PowerOffAccOn,
    e_PowerOffToPowerOn,
    e_PowerOffToAccPowerOn,
    e_PowerOn,
    e_PowerOnToPowerOff
} PowerStage_t;

typedef
enum PowerExtStage_e
{
    e_ExtPowerOff = 10,
    e_ExtPowerOn,
    e_ExtPowerOffWithPowerKey
} PowerExtStage_t;

static PowerStage_t powerStage = e_PowerOff;
static PowerExtStage_t powerExtStage = e_ExtPowerOff;

/* -------------------------------------------------------------------- */
static uint16_t status = 0;

#define STATUS_ADC_UPDATED      0x8000
#define STATUS_KEY_EVENT        0x4000
#define STATUS_KEY_TUNENEXT     0x0001
#define STATUS_KEY_TUNEPREV     0x0002
#define STATUS_KEY_VOLUMEUP     0x0004
#define STATUS_KEY_VOLUMEDOWN   0x0008

/*
#define KEY_HOME            0x00000001
#define KEY_MUTE            0x00000002
#define KEY_POWER           0x00000010
#define KEY_PLAYPAUSE       0x00000020
#define KEY_RADIO           0x00000040
#define KEY_NAVIGATION      0x00000080

#define KEY_EQ              0x00000400
#define KEY_EJECT           0x00000800
*/
void MAIN_VolumeStepDown(void)
{
    status |= (STATUS_KEY_EVENT | STATUS_KEY_VOLUMEDOWN);
}

void MAIN_VolumeStepUp(void)
{
    status |= (STATUS_KEY_EVENT | STATUS_KEY_VOLUMEUP);
}

void MAIN_TuneStepBack(void)
{
    status |= (STATUS_KEY_EVENT | STATUS_KEY_TUNEPREV);
}

void MAIN_TuneStepForward(void)
{
    status |= (STATUS_KEY_EVENT | STATUS_KEY_TUNENEXT);
}

void MAIN_ReportAdcValue(void)
{
    status |= (STATUS_ADC_UPDATED);
}

/*
void MAIN_EqKeyPressed(void)
{
    keyStatus |= (KEY_EVENT_OCCURED | KEY_EQ);
}

void MAIN_SrcKeyPressed(void)
{
    keyStatus |= (KEY_EVENT_OCCURED | KEY_HOME);
}

void MAIN_NaviKeyPressed(void)
{
    keyStatus |= (KEY_EVENT_OCCURED | KEY_NAVIGATION);
}

void MAIN_BandKeyPressed(void)
{
    keyStatus |= (KEY_EVENT_OCCURED | KEY_RADIO);
}

void MAIN_MuteKeyPressed(void)
{
    keyStatus |= (KEY_EVENT_OCCURED | KEY_MUTE);
}

void MAIN_EjectKeyPressed(void)
{
    keyStatus |= (KEY_EVENT_OCCURED | KEY_EJECT);
}
*/
/* -------------------------------------------------------------------- */
void LED_Green(OnOff_t state)
{
    if(state == On)
        HAL_GPIO_ClrBit(GPIOA, GPIO_BIT11);
    else
        HAL_GPIO_SetBit(GPIOA, GPIO_BIT11);
}

void LED_Red(OnOff_t state)
{
    if(state == On)
        HAL_GPIO_ClrBit(GPIOA, GPIO_BIT10);
    else
        HAL_GPIO_SetBit(GPIOA, GPIO_BIT10);
}

void LED_Yellow(OnOff_t state)
{
    if(state == On)
        HAL_GPIO_ClrBit(GPIOA, GPIO_BIT12);
    else
        HAL_GPIO_SetBit(GPIOA, GPIO_BIT12);
}

void LED_Off(void)
{
    HAL_GPIO_SetBit(GPIOA, GPIO_BIT11);
    HAL_GPIO_SetBit(GPIOA, GPIO_BIT10);
    HAL_GPIO_SetBit(GPIOA, GPIO_BIT12);
}

OnOff_t ACC_GetStatus(void)
{
    return !HAL_GPIO_GetBit(GPIOA, GPIO_BIT8) ? On : Off;
}

OnOff_t PWR_GetKeyStatus(void)
{
    return !HAL_GPIO_GetBit(GPIOA, GPIO_BIT0) ? On : Off;
}

OnOff_t PWR_GetStatus(void)
{
    return HAL_GPIO_GetBit(GPIOB, GPIO_BIT3) ? On : Off;
}

void PWR_Cycle(OnOff_t state)
{
    if(state == On)
        HAL_GPIO_ClrBit(GPIOA, GPIO_BIT9);
    else
        HAL_GPIO_SetBit(GPIOA, GPIO_BIT9);
}

/* -------------------------------------------------------------------- */
void USER_HardFaultHandler(void)
{
    while(true)
    {
        LED_Red(On);
        __delay_cycles(16000000/3/4);
        LED_Red(Off);
        __delay_cycles(16000000/3/4);
    }
}

/* -------------------------------------------------------------------- */
int EntryPoint(void)
{
    uint32_t waitCounter;

    InputEncoders_Init();
    InputKeys_Init();
    I2CHCI_Init(0x70);

    while (true)
    {
        if(powerStage == e_PowerOn)
        {
            /* Ensure that ADC started and status updated */
            InputKeys_Update();

            if(status & (STATUS_KEY_EVENT | STATUS_ADC_UPDATED))
            {
                I2CHCI_SetStatus(status);
                status = 0;

                /* toggle MCU_IRQ */
                HAL_GPIO_ClrBit(GPIOB, GPIO_BIT5);
                OS_DelayMs(1);
                HAL_GPIO_SetBit(GPIOB, GPIO_BIT5);
            }
        } else {
            status = 0;
        }

        switch (powerStage)
        {
            case e_PowerOff:
            {
                powerExtStage = e_ExtPowerOff;

                LED_Off();

                if(ACC_GetStatus() == On)
                {
                    LED_Yellow(On);

                    OS_DelayMs(1000);

                    if(ACC_GetStatus() == On)
                    {
                        powerStage = e_PowerOffAccOn;
                        break;
                    }

                    LED_Yellow(Off);
                }
                
                /* Strange situation, power is present when system in PowerOff stage, turn off... */
                if(PWR_GetStatus() == On)
                    powerStage = e_PowerOnToPowerOff;
                break;
            }
            case e_PowerOffAccOn:
            {
                LED_Green(Off);

                if(ACC_GetStatus() == Off)
                {
                    powerStage = e_PowerOff;
                    break;
                }

                LED_Yellow(On);

                if(powerExtStage != e_ExtPowerOffWithPowerKey)
                {
                    powerStage = e_PowerOffToAccPowerOn;
                    break;
                }

                LED_Red(On);

                if(PWR_GetKeyStatus() == On)
                {
                    OS_DelayMs(1000);
                    
                    if(PWR_GetKeyStatus() == On)
                    {
                        powerStage = e_PowerOffToPowerOn;
                        break;
                    }
                }
                break;
            }
            case e_PowerOffToPowerOn:
            case e_PowerOffToAccPowerOn:
            {
                if(PWR_GetStatus() == Off)
                {
                    LED_Red(On);
                    PWR_Cycle(On);

                    while(PWR_GetStatus() == Off)
                        OS_DelayMs(100);

                    PWR_Cycle(Off);
                }

                LED_Yellow(Off);
                LED_Red(Off);

                powerStage = e_PowerOn;
                powerExtStage = e_ExtPowerOn;
                break;
            }
            case e_PowerOn:
            {
                LED_Green(On);

                if(PWR_GetStatus() == Off)
                {
                    powerStage = e_PowerOffAccOn;
                    powerExtStage = e_ExtPowerOffWithPowerKey;
                    break;
                }

                if(ACC_GetStatus() == Off)
                {
                    LED_Yellow(On);

                    waitCounter = 50; /* 5 seconds */
                    while(--waitCounter > 0)
                    {
                        if(ACC_GetStatus() != Off)
                            break;
                        OS_DelayMs(100);
                    }

                    if(waitCounter == 0)
                    {
                        powerStage = e_PowerOnToPowerOff;
                        break;
                    }

                    LED_Yellow(Off);
                }

                break;
            }
            case e_PowerOnToPowerOff:
            {
                LED_Red(On);
                PWR_Cycle(On);

                while(PWR_GetStatus() == On)
                    OS_DelayMs(100);

                PWR_Cycle(Off);
                LED_Red(Off);

                powerStage = e_PowerOff;
                break;
            }
        }

        __dmb();
        __dsb();
        __isb();
    }
}
