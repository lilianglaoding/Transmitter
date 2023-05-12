#ifndef _ADC_H_
#define _ADC_H_

#include "pico/stdlib.h"
#include "sys_config.h"


extern uint16_t volatile  pwmValues[CH_NUM];

void AdcInit();
void AdcStart();


#endif
