#ifndef _SPI_H_
#define _SPI_H_

#include <stdint.h>

int spi_init(const char* dev);
int spi_send_word(int fd, uint16_t word);

#endif