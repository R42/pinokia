#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include "modules.h"

#define SPI_LSB_FIRST_MSB   0
#define SPI_BITS_PER_WORD_8 8
#define SPI_SPEED_500KHZ    500 * 1000 // Raspberry Pi minimum
#define SPI_SPEED_3815HZ    3815 // Linux minimum
#define SPI_DELAY_0         0
#define SPI_CS_CHANGE 1 // Don't clear CS between messages
                        // Actually should be cleared according to the epson
                        // data sheets

const char* SPI_DEV = "/dev/spidev0.0";

uint32_t tx_word;
static struct spi_ioc_transfer tx = {
  .tx_buf        = 0,
  .rx_buf        = 0,
  .len           = 0,
  .delay_usecs   = SPI_DELAY_0,
  .speed_hz      = SPI_SPEED_3815HZ,
  .bits_per_word = SPI_BITS_PER_WORD_8,
  .cs_change     = SPI_CS_CHANGE
};

static int spi_init(const char* dev) {
  uint8_t  mode      = SPI_MODE_0; // WAT ?
  uint8_t  lsb_first = SPI_LSB_FIRST_MSB;
  uint8_t  bpw       = SPI_BITS_PER_WORD_8;
  uint32_t speed     = SPI_SPEED_500KHZ;

  int fd = open(dev, O_RDWR);

  if (fd < 0)
    return fd;

  if (ioctl(fd, SPI_IOC_WR_MODE         , &mode     ) < 0) return -1;
  if (ioctl(fd, SPI_IOC_WR_LSB_FIRST    , &lsb_first) < 0) return -1;
  if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bpw      ) < 0) return -1;
  if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ , &speed    ) < 0) return -1;

  return fd;
} 

static void send_word(int fd, uint32_t word) {
  // tx.bits_per_word = 9;
  tx.len       = 1;
  tx.cs_change = 0;

  if (ioctl(fd, SPI_IOC_MESSAGE(1), &tx) < 0) {
    perror("Error sending SPI message");
    exit(1);
  }
}

int main(int argc, char* argv[]) {
  // make sure SPI kernel modules are loaded
  loadSpiModules();

  tx.tx_buf = (uint32_t) &tx_word;

  int fd = spi_init(SPI_DEV);
  if (fd < 0) {
    fprintf(stderr, "Couldn't open device (%s)\n", SPI_DEV);
    return fd;
  }

  printf("Sending word...\n");
  int i;
  for (i = 0; i < 100; i++) {
    send_word(fd, 0x81422418);
  }
  sleep(1);
  printf("Sent.\n");

  close(fd);

  return 0;
}
