
#ifndef __INPUT_H_
#define __INPUT_H_

void InputEncoders_Init(void);
void InputKeys_Init(void);
void InputKeys_Update(void);

extern void MAIN_VolumeStepDown(void);
extern void MAIN_VolumeStepUp(void);
extern void MAIN_TuneStepBack(void);
extern void MAIN_TuneStepForward(void);
extern void MAIN_ReportAdcValue(void);

/*
extern void MAIN_EqKeyPressed(void);
extern void MAIN_SrcKeyPressed(void);
extern void MAIN_NaviKeyPressed(void);
extern void MAIN_BandKeyPressed(void);
extern void MAIN_MuteKeyPressed(void);
extern void MAIN_EjectKeyPressed(void);
*/

#endif /* __INPUT_H_ */
