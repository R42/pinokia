#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include "modules.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define SPI_MSB_FIRST       0
#define SPI_BITS_PER_WORD_8 8
#define SPI_BITS_PER_WORD_9 9
#define SPI_SPEED_500KHZ    500 * 1000 // Raspberry Pi minimum
#define SPI_SPEED_1MHZ      1000 * 1000
#define SPI_SPEED_3815HZ    3815 // Linux minimum
#define SPI_DELAY_0         0
#define SPI_DELAY_MIN       2980 // Minimum tested value
#define SPI_CS_CHANGE 1 // Don't clear CS between messages
                        // Actually should be cleared according to the epson
                        // data sheets

const uint32_t DELAY     = SPI_DELAY_0;
const uint32_t SPEED     = SPI_SPEED_3815HZ;
const uint32_t BPW       = SPI_BITS_PER_WORD_9;
const uint32_t CS_CHANGE = SPI_CS_CHANGE;

const char* SPI_DEV = "/dev/spidev0.0";

// 25 == EPSON NOP
uint16_t tx[] = {
  0x155, 0x0000, 0x0125, 0x0125,
};
uint16_t rx[ARRAY_SIZE(tx)] = {0, };

static struct spi_ioc_transfer transfer;

static int spi_init(const char* dev) {
  uint8_t  mode      = SPI_MODE_0; // => not SPI_CPOL and not SPI_CPHA
  uint8_t  lsb_first = SPI_MSB_FIRST;
  uint8_t  bpw       = BPW;
  uint32_t speed     = SPEED;

  int fd = open(dev, O_RDWR);

  if (fd < 0)
    return fd;

  if (ioctl(fd, SPI_IOC_WR_MODE         , &mode     ) < 0) return -1;
  // if (ioctl(fd, SPI_IOC_RD_MODE         , &mode     ) < 0) return -1;
  if (ioctl(fd, SPI_IOC_WR_LSB_FIRST    , &lsb_first) < 0) return -1;
  // if (ioctl(fd, SPI_IOC_RD_LSB_FIRST    , &lsb_first) < 0) return -1;
  if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bpw      ) < 0) return -1;
  // if (ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bpw      ) < 0) return -1;
  if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ , &speed    ) < 0) return -1;
  // if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ , &speed    ) < 0) return -1;

  printf("spi mode: %d\n"             , mode);
  printf("*lsb first: %d\n"           , lsb_first);
  printf("bits per word: %d\n"        , bpw);
  printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);

  return fd;
} 

static uint32_t send_word(int fd, uint32_t word) {
  uint32_t resp = 0;

  struct spi_ioc_transfer tr = {
    .tx_buf = (unsigned long) tx,
    .rx_buf = (unsigned long) 0,//rx,
    .len = 4, //ARRAY_SIZE(tx),
    .delay_usecs = DELAY,
    .speed_hz = SPEED,
    .bits_per_word = BPW,
    .cs_change = 1,
  };
 
  // tr.tx_buf    = (unsigned long) tx;
  // tr.rx_buf    = (unsigned long) rx;
  // tr.len       = 2;
  // tr.delay_usecs = 100;
  // tr.speed_hz    = SPEED;
  // tr.bits_per_word = 9;
  // // tr.bits_per_word = 9;
  // tr.cs_change = 1;

  if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
    perror("Error sending SPI message");
    exit(1);
  }

  return resp;
}

int main(int argc, char* argv[]) {
  // make sure SPI kernel modules are loaded
  // loadSpiModules();

  int fd = spi_init(SPI_DEV);
  if (fd < 0) {
    fprintf(stderr, "Couldn't open device (%s)\n", SPI_DEV);
    return fd;
  }

  printf("Sending word...\n");
  int i;
  for (i = 0; i < 2; i++) {
    send_word(fd, 0xAA28);
  }
  sleep(1);
  printf("Sent.\n");

  close(fd);

  return 0;
}
