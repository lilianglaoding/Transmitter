#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "ppm.h"
#include "adc.h"
#include "sys_config.h"


#define PPM_PIN                22

extern uint16_t volatile pwmValues[CH_NUM];
uint16_t volatile ppmValues[PPM_NUM] = { MS05, 500, MS05, 1000, MS05, 1000, MS05, 1000, MS05, 1000, MS05, 1000, MS05, 1000, MS05, 1000, MS05, 8000 };
uint16_t volatile PPM_Index = 0;
uint8_t volatile ppmValue = 0;
uint16_t pwmSum = 0;


void PPMInit()
{
    gpio_init(PPM_PIN);
    gpio_set_dir(PPM_PIN, GPIO_OUT);
    gpio_put(PPM_PIN, ppmValue);
}


int64_t alarmCallback(alarm_id_t id, void *user_data)
{
    gpio_put(PPM_PIN, ppmValue);

    add_alarm_in_us(ppmValues[PPM_Index], alarmCallback, NULL, false);

    ppmValue = !ppmValue;
    ++PPM_Index;
    if (PPM_Index == PPM_NUM)
    {
        PPM_Index = 0;
        pwmSum = 0;
    }

    if (PPM_Index % 2) {
        if (PPM_Index != PPM_NUM - 1)
        {
            ppmValues[PPM_Index] = pwmValues[(PPM_Index - 1) / 2] - MS05;
            pwmSum += pwmValues[(PPM_Index - 1) / 2];
        }
        else {
            ppmValues[PPM_Index] = MS20 - pwmSum;
        }
    }

    return 0;
}

void PPMStart()
{
    PPM_Index = 0;
    ppmValue = 0;
    pwmSum = 0;

    gpio_put(PPM_PIN, ppmValue);

    add_alarm_in_us(ppmValues[PPM_Index], alarmCallback, NULL, false);
    
    ppmValue = !ppmValue;
    ++PPM_Index;

    ppmValues[PPM_Index] = pwmValues[(PPM_Index - 1) / 2] - MS05;
    pwmSum += pwmValues[(PPM_Index - 1) / 2];
}
