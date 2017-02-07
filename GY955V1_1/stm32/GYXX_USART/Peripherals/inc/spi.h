#ifndef __SPI_H
#define __SPI_H
#include "stm32f10x.h"
void Spi1_Int(uint16_t BaudRatePrescaler);
void Spi_write_buf(uint8_t add, uint8_t *pBuf, uint8_t num);
void Spi_read_buf(uint8_t add, uint8_t *pBuf, uint8_t num);
#endif
