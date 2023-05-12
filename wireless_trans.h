#ifndef _WIRELESS_TRANS_H_
#define _WIRELESS_TRANS_H_


#define RADIO_nRESET_PIN       5

#define RADIO_BUSY_PIN         6

#define RADIO_DIOx_PIN         4

#define RADIO_CTX_PIN          10
#define RADIO_CPS_PIN          9
#define RADIO_CSD_PIN          8

#define ANT_SEL_PIN            7


void WRTInit(void);
void StartSendPacket();


#endif

