#ifndef _SPI_H_
#define _SPI_H_

static uint32_t send_word(int fd, uint32_t word);

static int spi_init(const char* dev);

#endif