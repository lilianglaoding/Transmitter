#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "beep.h"

uint slice_num_beep;

#define BEEP_PIN               11

int freqsOnSystemStart[] = { 698, 784, 880, 988 };
int freqsOnSwitchToEmulator[] = { 370, 392, 451, 440 };
int freqsOnSwitchToRemoter[] = { 523, 554, 587, 622 };
int freqsOnPacketTxDone[] = {470, 470, 470, 470};
int freqsOnPacketRxDone[] = {490, 490, 490, 490};
int freqsOnPacketTxTimeout[] = {3800, 3800, 3800, 3800};
int freqsOnPacketRxTimeout[] = {4000, 4000, 4000, 4000};
int freqsOnPacketRxError[] = {4100, 4100, 4100, 4100};
int freqsOnWRTRangingDone[] = {3700, 3700, 3700, 3700};
int freqsOnWRTCadDone[] = {510, 510, 510, 510};
int freqsOnReceiverVoltageLessThan20[] = { 1976, 1760, 1568, 1397 };
int freqsOnReceiverVoltageLessThan10[] = { 3951, 3520, 3136, 2960 };


void BeepInit()
{
    /****************************************
     BEEP_PIN
    ****************************************/
    gpio_set_function(BEEP_PIN, GPIO_FUNC_PWM);
    slice_num_beep = pwm_gpio_to_slice_num(BEEP_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 249.f);
    pwm_init(slice_num_beep, &config, false);
    pwm_set_wrap(slice_num_beep, 8000);
    pwm_set_gpio_level(BEEP_PIN, 1);
    pwm_set_enabled(slice_num_beep, false);
}

void BeepOnState(BeepState_t state)
{
    int *freqs = freqsOnSystemStart;

    switch (state)
    {
    case systemStart:
        freqs = freqsOnSystemStart;
        break;
    case switchToEmulator:
        freqs = freqsOnSwitchToEmulator;
        break;
    case switchToRemoter:
        freqs = freqsOnSwitchToRemoter;
        break;
    case packetTxDone:
        freqs = freqsOnPacketTxDone;
        break;
    case packetRxDone:
        freqs = freqsOnPacketRxDone;
        break;
    case packetTxTimeout:
        freqs = freqsOnPacketTxTimeout;
        break;
    case packetRxTimeout:
        freqs = freqsOnPacketRxTimeout;
        break;
    case packetRxError:
        freqs = freqsOnPacketRxError;
        break;
    case WRTRangingDone:
        freqs = freqsOnWRTRangingDone;
        break;
    case WRTCadDone:
        freqs = freqsOnWRTCadDone;
        break;
    case ReceiverVoltageLessThan20:
        freqs = freqsOnReceiverVoltageLessThan20;
        break;
    case ReceiverVoltageLessThan10:
        freqs = freqsOnReceiverVoltageLessThan10;
        break;
    default:
        break;
    }

    if (freqs == NULL) {
        return ;
    }
    
    pwm_set_enabled(slice_num_beep, true);
    
    for (int i = 0; i < 4; i++)
    {
        uint16_t wrap = 125000000 / 250 / freqs[i];
        pwm_set_wrap(slice_num_beep, wrap);
        pwm_set_gpio_level(BEEP_PIN, 400);
        sleep_ms(300);
    }
    pwm_set_gpio_level(BEEP_PIN, 1);
    pwm_set_enabled(slice_num_beep, false);
}
