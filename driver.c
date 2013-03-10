#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include <errno.h>
#include <stdio.h>

#include <time.h>
#include <stdlib.h>

#include "driver.h"

#define LCD_SPI_BITS_PER_WORD 9
#define LCD_SPI_MODE SPI_MODE_0
// #define LCD_SPI_SPEED 3815 // Minimum
// #define LCD_SPI_SPEED 3000000 // From the LCD datasheet
#define LCD_SPI_SPEED 100000

static int spi_init(char * dev) {
  int fd;

  uint8_t mode = LCD_SPI_MODE;
  uint8_t bpw = 8; // has to be initialized with 8 - https://www.kernel.org/doc/Documentation/spi/spidev_test.c
  uint32_t speed = LCD_SPI_SPEED;


  fd = open(dev, O_RDWR);

  if (fd < 0)
    return fd;

  if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0)
    return -1;

  if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bpw) < 0)
    return -1;

  if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0)
    return -1;

  return fd;
}

static void send_cmd(LCD *lcd,
                     uint8_t cmd,
                     unsigned char *data,
                     unsigned int data_len) {

  uint32_t stride = 1;//2 * 1024; // Max = 2048
  uint32_t len, i;
  uint16_t buf[1 + data_len];
  struct spi_ioc_transfer tx;

  buf[0] = cmd;
  for (i = 0; i < data_len; ++i)
    buf[i + 1] = data[i] | 0x0100;
  len = data_len + 1;

  bzero(&tx, sizeof(struct spi_ioc_transfer));
  tx.tx_buf = (uint32_t) buf;
  tx.rx_buf = (uint32_t) 0;
  tx.len = 2;//len;
  tx.bits_per_word = LCD_SPI_BITS_PER_WORD;
  tx.speed_hz = LCD_SPI_SPEED;
  tx.delay_usecs = (uint16_t) 0;

  for (i = 0; i < len; i += stride) {
    tx.tx_buf = (uint32_t) &buf[i];
    tx.len = 2 * (i + stride > len ? len - i : stride);

    if (ioctl(lcd->fd, SPI_IOC_MESSAGE(1), &tx) < 0) {
      perror("Error sending SPI message");
      exit(1);
    }
  }
}

int lcd_clear(LCD *lcd, int color) {
  uint8_t paset, caset, ramwr;

  uint32_t ramwr_data_len = ((131 * 131) / 2) * 3;

  uint8_t xaset_data[] = { 0, 131 };
  uint8_t ramwr_data[ramwr_data_len];


  uint32_t i;

  for(i=0; i < ramwr_data_len; i+=3) {
    color = rand();
    ramwr_data[i+0] = (color >> 4) & 0xFF;
    ramwr_data[i+1] = ((color & 0x0F) << 4) | (color >> 8);
    ramwr_data[i+2] = color & 0x0FF;
  }

  if (lcd->type == TYPE_EPSON) {
    paset = PASET;
    caset = CASET;
    ramwr = RAMWR;
  } else {
    paset = PASETP;
    caset = CASETP;
    ramwr = RAMWRP;
  }

  send_cmd(lcd, paset, xaset_data, 2);
  send_cmd(lcd, caset, xaset_data, 2);
  send_cmd(lcd, ramwr, ramwr_data, 0);

  return 0;
}

int lcd_init(LCD *lcd, char *dev, int type) {

  bzero(lcd, sizeof(LCD));
  lcd->dev = dev;
  lcd->type = type;
  lcd->fd = spi_init(dev);
  if (lcd->fd < 0)
    return lcd->fd;

  // display control
  // 12 = 1100 - CL dividing ratio [don't divide] switching period 8H (default)
  unsigned char disctl_data[] = { 0x0c, 0x20, 0x00, 0x01 };
  send_cmd(lcd, DISCTL, disctl_data, sizeof(disctl_data));

  // common scanning direction
  unsigned char comscn_data[] = { 0x01 };
  send_cmd(lcd, COMSCN, comscn_data, sizeof(comscn_data));

  // internal oscillator ON
  send_cmd(lcd, OSCON, NULL, 0);

  // sleep out
  send_cmd(lcd, SLPOUT, NULL, 0);

  // power ctrl
  // everything on, no external reference resistors
  unsigned char pwrctr_data[] = { 0x0F };
  send_cmd(lcd, PWRCTR, pwrctr_data, sizeof(pwrctr_data));

  // invert display mode
  send_cmd(lcd, DISINV, NULL, 0);

  // data control
  // - 0x03 - correct for normal sin7
  // - 0x00 - normal RGB arrangement
  // - 0x02 - 16-bit Grayscale Type A
  unsigned char datctl_data[] = { 0x03, 0x00, 0x02, 0xc8, 0x02 };
  send_cmd(lcd, DATCTL, datctl_data, sizeof(datctl_data));

  // electronic volume, this is the contrast/brightness
  // - 0x24 - volume (contrast) setting - fine tuning, original
  // - 0x03 - internal resistor ratio - coarse adjustment
  unsigned char volctr_data[] = { 0x24, 0x03, 0x30 };
  send_cmd(lcd, VOLCTR, volctr_data, sizeof(volctr_data));

  // nop
  send_cmd(lcd, NOP, NULL, 0);

  usleep(200 * 1000);

  // display on
  send_cmd(lcd, DISON, NULL, 0);

  return 0;
}

void lcd_dispose(LCD *lcd) {
  close(lcd->fd);
}
