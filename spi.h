#ifndef  _SPI_H_
#define  _SPI_H_

#include "pico/stdlib.h"

void SPIInit(void);
void SpiInOut( uint8_t *txBuffer, uint8_t *rxBuffer, uint16_t size);
void SpiIn( uint8_t *txBuffer, uint16_t size );

#endif

