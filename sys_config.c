#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "sys_config.h"

bool PPMMode = false;


/*
void SwitchModePinCallback(uint gpio, uint32_t events)
{
    printf("mode changed\n");
    PPMMode = !PPMMode;
    OledOnSystemModeChanged(PPMMode);
    //BeepOnModeChanged();
    //SwitchBeepState(PPMMode);
    SwitchMode(PPMMode);
    //BeepOnState(switchToEmulator);
}

void RegisterSwitchSysModeIrq()
{
    gpio_init(MODE_SWITCH_PIN);
    gpio_set_dir(MODE_SWITCH_PIN, GPIO_IN);
    gpio_pull_up(MODE_SWITCH_PIN);

    gpio_set_irq_enabled_with_callback(MODE_SWITCH_PIN, GPIO_IRQ_EDGE_FALL, true, &SwitchModePinCallback);
}
*/

void SystemInit()
{
    gpio_init(MODE_SWITCH_PIN);
    gpio_set_dir(MODE_SWITCH_PIN, GPIO_IN);
    gpio_pull_up(MODE_SWITCH_PIN);
    sleep_ms(50);
    PPMMode = !gpio_get(MODE_SWITCH_PIN);
}
