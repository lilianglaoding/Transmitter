#ifndef _OLED_H_
#define _OLED_H_


void OLEDInit();
void OledOnSystemStart();
void OledOnVoltageChanged(uint8_t bat1Percentage, uint8_t bat2Percentage);
void OledOnSystemModeChanged(bool sysMode);


#endif
