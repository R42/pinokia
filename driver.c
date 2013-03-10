#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <errno.h>

#include "driver.h"

#define LCD_SPI_BITS_PER_WORD 9
#define LCD_SPI_MODE SPI_MODE_0
#define LCD_SPI_SPEED 1
// #define LCD_SPI_SPEED 3000000

static int spi_init(char * dev) {
  int fd;

  uint8_t mode = LCD_SPI_MODE;
  uint8_t bpw = LCD_SPI_BITS_PER_WORD;
  uint32_t speed = LCD_SPI_SPEED;


  fd = open(dev, O_RDWR);

  if (fd < 0)
    return fd;

  if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0)
    return -1;
  if (ioctl(fd, SPI_IOC_RD_MODE, &mode) < 0)
    return -1;

  if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bpw) < 0) {
    printf("ERRNO: %x\n", errno);
    return -1;
  }
  if (ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bpw) < 0)
    return -1;

  if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0)
    return -1;
  if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0)
    return -1;

  return fd;
}

static void add_cmd(
  struct spi_ioc_transfer **ptx,
  uint16_t **pbuf,
  char cmd,
  unsigned char *data,
  unsigned int data_len
  ) {

  struct spi_ioc_transfer *tx;
  uint16_t *buf;
  uint32_t len;
  int i;

  if (!data)
    data_len = 0;

  len = 2 * (1 + data_len);

  tx = *ptx;
  buf = *pbuf;

  bzero(tx, sizeof(struct spi_ioc_transfer));
  tx->tx_buf = (uint32_t) buf;
  tx->len = len;

  ptx += 1;
  pbuf += len;

  buf[0] = ((uint16_t)cmd & 0x00FF);
  for (i=0; i<data_len; ++i)
    buf[i+1] = ((uint16_t)(data[i]) & 0x00FF) | 0x0100;
}


int lcd_clear(LCD *lcd, int color) {
  uint8_t paset, caset, ramwr;

  uint32_t ramwr_data_len = ((131 * 131) / 2) * 3;

  uint8_t xaset_data[] = { 0, 131 };
  uint8_t ramwr_data[ramwr_data_len];

  struct spi_ioc_transfer tx[3], *ptx;
  uint16_t buf[1+2 + 1+2 + 1+ramwr_data_len], *pbuf;

  uint32_t i;

  for(i=0; i < ramwr_data_len; i+=3) {
    ramwr_data[i+0] = (color >> 4) & 0xFF;
    ramwr_data[i+1] = ((color & 0x0F) << 4) | (color >> 8);
    ramwr_data[i+2] = color & 0x0FF;
  }

  ptx = tx;
  pbuf = buf;

  if (lcd->type == TYPE_EPSON) {
    paset = PASET;
    caset = CASET;
    ramwr = RAMWR;
  } else {
    paset = PASETP;
    caset = CASETP;
    ramwr = RAMWRP;
  }

  add_cmd(&ptx, &pbuf, paset, xaset_data, 2);
  add_cmd(&ptx, &pbuf, caset, xaset_data, 2);
  add_cmd(&ptx, &pbuf, ramwr, ramwr_data, 0);

  return ioctl(lcd->fd, SPI_IOC_MESSAGE(ptx - tx), tx);
}

int lcd_init(LCD *lcd, char *dev, int type) {
  struct spi_ioc_transfer tx[10], *ptx;
  uint16_t buf[1+4 + 1+1 + 1 + 1 + 1+1 + 1 + 1+5 + 1+3 + 1 + 1], *pbuf;

  ptx = tx;
  pbuf = buf;

  bzero(lcd, sizeof(LCD));
  lcd->dev = dev;
  lcd->type = type;
  lcd->fd = spi_init(dev);
  if (lcd->fd < 0)
    return lcd->fd;

  // display control
  // 12 = 1100 - CL dividing ratio [don't divide] switching period 8H (default)
  unsigned char disctl_data[] = { 0x0c, 0x20, 0x00, 0x01 };
  add_cmd(&ptx, &pbuf, DISCTL, disctl_data, sizeof(disctl_data));

  // common scanning direction
  unsigned char comscn_data[] = { 0x01 };
  add_cmd(&ptx, &pbuf, COMSCN, comscn_data, sizeof(comscn_data));

  // internal oscialltor ON
  add_cmd(&ptx, &pbuf, OSCON, NULL, 0);

  // sleep out
  add_cmd(&ptx, &pbuf, SLPOUT, NULL, 0);

  // power ctrl
  // everything on, no external reference resistors
  unsigned char pwrctr_data[] = { 0x0F };
  add_cmd(&ptx, &pbuf, PWRCTR, pwrctr_data, sizeof(pwrctr_data));

  // invert display mode
  add_cmd(&ptx, &pbuf, DISINV, NULL, 0);

  // data control
  // - 0x03 - correct for normal sin7
  // - 0x00 - normal RGB arrangement
  // - 0x02 - 16-bit Grayscale Type A
  unsigned char datctl_data[] = { 0x03, 0x00, 0x02, 0xc8, 0x02 };
  add_cmd(&ptx, &pbuf, DATCTL, datctl_data, sizeof(datctl_data));

  // electronic volume, this is the contrast/brightness
  // - 0x24 - volume (contrast) setting - fine tuning, original
  // - 0x03 - internal resistor ratio - coarse adjustment
  unsigned char volctr_data[] = { 0x24, 0x03, 0x30 };
  add_cmd(&ptx, &pbuf, VOLCTR, volctr_data, sizeof(volctr_data));

  // nop
  add_cmd(&ptx, &pbuf, NOP, NULL, 0);

  usleep(200);

  // display on
  add_cmd(&ptx, &pbuf, DISON, NULL, 0);

  return ioctl(lcd->fd, SPI_IOC_MESSAGE(ptx - tx), tx);
}

void lcd_dispose(LCD *lcd) {
  close(lcd->fd);
}
