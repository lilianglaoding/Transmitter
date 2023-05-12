#include <stdio.h>
#include "pico/multicore.h"
#include "sys_config.h"
#include "adc.h"
#include "oled.h"
#include "beep.h"
#include "wireless_trans.h"
#include "ppm.h"
#include "spi.h"

void (*TaskInit)() = WRTInit;
void (*TaskStart)() = StartSendPacket;

void HW_Int()
{
    BeepInit();
    BeepOnState(systemStart);
    OLEDInit();
    OledOnSystemStart();

    AdcInit();
    
    SystemInit();

    if (PPMMode) {
        TaskInit = PPMInit;
        TaskStart = PPMStart;
        BeepOnState(switchToEmulator);
    }
    else {
        TaskInit = WRTInit;
        TaskStart = StartSendPacket;
        BeepOnState(switchToRemoter);
    }
    TaskInit();
    OledOnSystemModeChanged(PPMMode);
}


/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{
    stdio_init_all();

    HW_Int();
    
    multicore_launch_core1(TaskStart);

    AdcStart();

    return 0;    
}
