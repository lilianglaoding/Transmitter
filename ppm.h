#ifndef _PPM_H_
#define _PPM_H_


#include "pico/stdlib.h"
#include "sys_config.h"

#define  MS20                  20000
#define  MS05                  500
#define  PPM_NUM               (CH_NUM * 2 + 2)


extern uint16_t volatile ppmValues[PPM_NUM];


void PPMStart();
void PPMInit();


#endif
