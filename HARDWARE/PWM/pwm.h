#ifndef _TIMER_H
#define _TIMER_H
#include "sys.h"

void TIM3_PWM_Init(u32 arr,u32 psc);
void TIM8_PWM_Init();
void check(u8 len);
void rFail(void);
int Check_per(u8 len);
int Check_fre(u8 len);

#endif
