#ifndef _BEEP_H_
#define _BEEP_H_

typedef enum {
    systemStart,
    switchToEmulator,
    switchToRemoter,
    packetTxDone,
    packetRxDone,
    packetTxTimeout,
    packetRxTimeout,
    packetRxError,
    WRTRangingDone,
    WRTCadDone,
    ReceiverVoltageLessThan20,
    ReceiverVoltageLessThan10,
} BeepState_t;


void BeepInit();
void BeepOnState(BeepState_t state);

#endif
