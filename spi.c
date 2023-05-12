#include "spi.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "pico/binary_info.h"

#define SPI_PORT spi0

#define RADIO_NSS_PIN          1
#define RADIO_MOSI_PIN         3
#define RADIO_MISO_PIN         0
#define RADIO_SCK_PIN          2


static inline void cs_select(uint cs_pin) {
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(cs_pin, 0);
    asm volatile("nop \n nop \n nop"); // FIXME
}

static inline void cs_deselect(uint cs_pin) {
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(cs_pin, 1);
    asm volatile("nop \n nop \n nop"); // FIXME
}

void SPIInit()
{
    /*********************************************************
     RADIO_NSS_PIN RADIO_MOSI_PIN RADIO_MISO_PIN RADIO_SCK_PIN 
    **********************************************************/
    spi_init(SPI_PORT, 1000 * 1000);
    gpio_set_function(RADIO_MISO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(RADIO_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(RADIO_MOSI_PIN, GPIO_FUNC_SPI);
    //gpio_set_function(RADIO_NSS_PIN, GPIO_FUNC_SPI);
    //bi_decl(bi_3pins_with_func(RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_SCK_PIN, RADIO_NSS_PIN, GPIO_FUNC_SPI));
    bi_decl(bi_3pins_with_func(RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_SCK_PIN, GPIO_FUNC_SPI));
    gpio_init(RADIO_NSS_PIN);
    gpio_set_dir(RADIO_NSS_PIN, GPIO_OUT);
    gpio_put(RADIO_NSS_PIN, 1);
    // Make the CS pin available to picotool
    bi_decl(bi_1pin_with_name(RADIO_NSS_PIN, "SPI CS"));
}

/*!
 * @brief Sends txBuffer and receives rxBuffer
 *
 * @param [IN] txBuffer Byte to be sent
 * @param [OUT] rxBuffer Byte to be sent
 * @param [IN] size Byte to be sent
 */
void SpiInOut( uint8_t *txBuffer, uint8_t *rxBuffer, uint16_t size)
{
    cs_select(RADIO_NSS_PIN);
    spi_write_read_blocking(SPI_PORT, txBuffer, rxBuffer, size);
    cs_deselect(RADIO_NSS_PIN);
}

void SpiIn( uint8_t *txBuffer, uint16_t size )
{
    cs_select(RADIO_NSS_PIN);
    spi_write_blocking(SPI_PORT, txBuffer, size);
    cs_deselect(RADIO_NSS_PIN);
}


