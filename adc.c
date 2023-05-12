#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "adc.h"

#include "sys_config.h"


#define ROCKER_Y1_ADC_PIN      26
#define ROCKER_X1_ADC_PIN      27
#define ROCKER_Y2_ADC_PIN      28
#define ROCKER_X2_ADC_PIN      29

#define ROCKER_Y1_DIR_PIN      12
#define ROCKER_X1_DIR_PIN      13
#define ROCKER_Y2_DIR_PIN      14
#define ROCKER_X2_DIR_PIN      15

#define CHANNEL_1_PIN          18
#define CHANNEL_2_PIN          19
#define CHANNEL_3_PIN          20
#define CHANNEL_4_PIN          21

#define PWM_MIN                1000
#define PWM_MID                1500
#define PWM_MAX                2000


uint16_t volatile chValues[CH_NUM];
uint16_t volatile pwmValues[CH_NUM];
//extern uint16_t volatile ppmValues[PPM_NUM];


void AdcInit()
{
    /********************************************************
     ROCKER_X1_PIN ROCKER_Y1_PIN ROCKER_X2_PIN ROCKER_Y2_PIN
    *********************************************************/
    adc_init();
    adc_gpio_init(ROCKER_Y1_ADC_PIN);
    adc_gpio_init(ROCKER_X1_ADC_PIN);
    adc_gpio_init(ROCKER_Y2_ADC_PIN);
    adc_gpio_init(ROCKER_X2_ADC_PIN);

    /****************************************
     CHANNEL_1_PIN CHANNEL_2_PIN CHANNEL_3_PIN CHANNEL_4_PIN
    ****************************************/
    gpio_init(CHANNEL_1_PIN);
    gpio_set_dir(CHANNEL_1_PIN, GPIO_IN);
    // We are using the button to pull down to 0v when pressed, so ensure that when
    // unpressed, it uses internal pull ups. Otherwise when unpressed, the input will
    // be floating.
    gpio_pull_up(CHANNEL_1_PIN);
    gpio_init(CHANNEL_2_PIN);
    gpio_set_dir(CHANNEL_2_PIN, GPIO_IN);
    gpio_pull_up(CHANNEL_2_PIN);
    gpio_init(CHANNEL_3_PIN);
    gpio_set_dir(CHANNEL_3_PIN, GPIO_IN);
    gpio_pull_up(CHANNEL_3_PIN);
    gpio_init(CHANNEL_4_PIN);
    gpio_set_dir(CHANNEL_4_PIN, GPIO_IN);
    gpio_pull_up(CHANNEL_4_PIN);

    /****************************************
     ROCKER_Y1_DIR_PIN ROCKER_X1_DIR_PIN ROCKER_Y2_DIR_PIN ROCKER_X2_DIR_PIN
    ****************************************/
    gpio_init(ROCKER_Y1_DIR_PIN);
    gpio_set_dir(ROCKER_Y1_DIR_PIN, GPIO_IN);
    // We are using the button to pull down to 0v when pressed, so ensure that when
    // unpressed, it uses internal pull ups. Otherwise when unpressed, the input will
    // be floating.
    gpio_pull_up(ROCKER_Y1_DIR_PIN);
    gpio_init(ROCKER_X1_DIR_PIN);
    gpio_set_dir(ROCKER_X1_DIR_PIN, GPIO_IN);
    gpio_pull_up(ROCKER_X1_DIR_PIN);
    gpio_init(ROCKER_Y2_DIR_PIN);
    gpio_set_dir(ROCKER_Y2_DIR_PIN, GPIO_IN);
    gpio_pull_up(ROCKER_Y2_DIR_PIN);
    gpio_init(ROCKER_X2_DIR_PIN);
    gpio_set_dir(ROCKER_X2_DIR_PIN, GPIO_IN);
    gpio_pull_up(ROCKER_X2_DIR_PIN);
}


float map(float value, float fromLow, float fromHigh, float toLow, float toHigh)
{
    return ((value - fromLow ) * (toHigh - toLow) / (fromHigh - fromLow) + toLow);
}


int mapChValue(int val, int lower, int middle, int upper, int reverse)
{
    if(val > upper)
        val = upper;
        
    if(val < lower)
        val = lower;
        
    if ( val < middle )
    {
    	val = (int)map(val, lower, middle, PWM_MIN, PWM_MID);
    }
    else
    {
    	val = (int)map(val, middle, upper, PWM_MID, PWM_MAX);
    }
    return ( reverse ? (PWM_MIN + PWM_MAX) - val : val );
}


void AdcStart()
{
    uint16_t pwmSum = 0;

    while (1) {
        for(uint16_t chIndex = 0; chIndex < 4; chIndex++)
	    {
            adc_select_input(chIndex);
            chValues[chIndex] = adc_read();
	    }
	    
        for(uint16_t chIndex = 4; chIndex < CH_NUM; chIndex++)
        {
            chValues[chIndex] = (gpio_get(chIndex + 14) + 1) * 2047;
        }

        for(uint16_t chIndex = 0; chIndex < 4; chIndex++)
        {
            pwmValues[chIndex] = mapChValue(chValues[chIndex], 0, 2047, 4095, gpio_get(12 + chIndex));
        }

        for(uint16_t chIndex = 4; chIndex < CH_NUM; chIndex++)
        {
            pwmValues[chIndex] = mapChValue(chValues[chIndex], 0, 2047, 4095, 1);
        }
    }
}
