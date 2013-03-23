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

// https://www.kernel.org/doc/Documentation/spi/spidev

#define LCD_SPI_MODE      SPI_MODE_0
#define LCD_SPI_LSB_FIRST 0 // MSB-first

#define LCD_SPI_INIT_BITS_PER_WORD 8
#define LCD_SPI_BITS_PER_WORD 9

// #define LCD_SPI_SPEED 3815 // Tested minimum
// #define LCD_SPI_SPEED 3 * 1000 * 1000 // From the LCD datasheet
#define LCD_SPI_SPEED 1000 * 1000

//#define LCD_SPI_STRIDE 2048 // Tested maximum
#define LCD_SPI_STRIDE 1

#define LCD_SPI_DELAY_USECS 0

#define LCD_SPI_CS_CHANGE 1 // Don't clear CS between messages

static struct spi_ioc_transfer tx = {
  .tx_buf        = 0,
  .rx_buf        = 0,
  .len           = 0,
  .delay_usecs   = LCD_SPI_DELAY_USECS,
  .speed_hz      = LCD_SPI_SPEED,
  .bits_per_word = LCD_SPI_BITS_PER_WORD,
  .cs_change     = LCD_SPI_CS_CHANGE,
};

static int spi_init(char * dev) {
  int fd;

  uint8_t mode = LCD_SPI_MODE;
  uint8_t lsb_first = LCD_SPI_LSB_FIRST;
  // has to be initialized with 8 (LCD_SPI_INIT_BITS_PER_WORD)
  // @see https://www.kernel.org/doc/Documentation/spi/spidev_test.c
  uint8_t bpw = LCD_SPI_INIT_BITS_PER_WORD;
  uint32_t speed = LCD_SPI_SPEED;


  fd = open(dev, O_RDWR);

  if (fd < 0)
    return fd;

  if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0)
    return -1;

  if (ioctl(fd, SPI_IOC_WR_LSB_FIRST, &lsb_first) < 0)
    return -1;

  if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bpw) < 0)
    return -1;

  if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0)
    return -1;

  return fd;
}

static void send_cmd(LCD *lcd, uint8_t cmd) {
  uint16_t cmd_word = cmd;

  tx.tx_buf = (uint32_t) &cmd_word;
  tx.len    = 2;

  if (ioctl(lcd->fd, SPI_IOC_MESSAGE(1), &tx) < 0) {
    perror("Error sending SPI message");
    exit(1);
  }
}

static void send_data(LCD *lcd, uint8_t data) {
  uint16_t data_word = 0x100 | data;

  tx.tx_buf = (uint32_t) &data_word;
  tx.len    = 2;

  if (ioctl(lcd->fd, SPI_IOC_MESSAGE(1), &tx) < 0) {
    perror("Error sending SPI message");
    exit(1);
  }
}

int lcd_clear(LCD *lcd, int color) {
  uint8_t paset, caset, ramwr;
  uint32_t i;

  if (lcd->type == TYPE_EPSON) {
    paset = PASET;
    caset = CASET;
    ramwr = RAMWR;
  } else {
    paset = PASETP;
    caset = CASETP;
    ramwr = RAMWRP;
  }

  send_cmd(lcd, paset);
  send_data(lcd, 0);
  send_data(lcd, 131);
  send_cmd(lcd, caset);
  send_data(lcd, 0);
  send_data(lcd, 131);
  send_cmd(lcd, ramwr);

  for(i = 0; i < (131 * 131) / 2; i++) {
    color = (i * 2) & 0xfff;
    send_data(lcd, (color >> 4) & 0xFF);
    send_data(lcd, ((color & 0x0F) << 4) | (color >> 8));
    send_data(lcd, color & 0x0FF);
    if (i % 131 == 0) {
      printf(".");
      fflush(stdout);
    }
  }
  printf("\n");


  return 0;
}

int lcd_set_pixel(LCD *lcd, uint8_t x, uint8_t y, uint16_t color) {
  uint8_t paset, caset, ramwr;

  if (lcd->type == TYPE_EPSON) {
    paset = PASET;
    caset = CASET;
    ramwr = RAMWR;
  } else {
    paset = PASETP;
    caset = CASETP;
    ramwr = RAMWRP;
  }

  send_cmd(lcd, paset); // page start/end ram
  send_data(lcd, x);
  send_data(lcd, ENDPAGE);

  send_cmd(lcd, caset); // column start/end ram
  send_data(lcd, y);
  send_data(lcd, ENDCOL);

  send_cmd(lcd, ramwr);


  if (lcd->type == TYPE_EPSON) {
    send_data(lcd, (color >> 4) & 0x00ff);
    send_data(lcd, ((color & 0x0f) << 4) | (color >> 8));
    send_data(lcd, color & 0x0ff);
    // send_data(lcd, color);
    // send_data(lcd, NOP);
    // send_data(lcd, NOP);
  } else {
    send_data(lcd, (uint8_t) ((color >> 4) & 0x00FF));
    send_data(lcd, (uint8_t) (((color & 0x0F) << 4) | 0x00));
  }

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
  send_cmd(lcd, DISCTL);
  send_data(lcd, 0x0c);  // 12 = 1100 - CL dividing ratio [don't divide] switching period 8H (default)
  send_data(lcd, 0x20);
  send_data(lcd, 0x00);

  send_data(lcd, 0x01);

  // common scanning direction
  send_cmd(lcd, COMSCN);
  send_data(lcd, 0x01);

  // internal oscillator ON
  send_cmd(lcd, OSCON);

  // sleep out (EPSON)
  send_cmd(lcd, SLPOUT);

  // sleep out (PHILIPS)
  send_cmd(lcd, SLEEPOUT);

  // power ctrl (EPSON)
  send_cmd(lcd, PWRCTR);
  send_data(lcd, 0x0F); // everything on, no external reference resistors

  // booset on (PHILLIPS)
  send_cmd(lcd, BSTRON);

  // invert display mode (EPSON)
  send_cmd(lcd, DISINV);

  // invert display mode (PHILLIPS)
  send_cmd(lcd, INVON);

  // data control (EPSON)
  send_cmd(lcd, DATCTL);
  send_data(lcd, 0x03); // correct for normal sin7
  send_data(lcd, 0x00); // normal RGB arrangement
  // send_data(lcd, 0x01); // 8-bit Grayscale [not used]
  send_data(lcd, 0x02); // 16-bit Grayscale Type A

  // memory access control (PHILLIPS)
  send_cmd(lcd, MADCTL);
  send_data(lcd, 0xc8);

  //  color mode (PHILLIPS)
  send_cmd(lcd, COLMOD);
  send_data(lcd, 0x02);

  // electronic volume, this is the contrast/brightness (EPSON)
  send_cmd(lcd, VOLCTR);
  send_data(lcd, 0x24); // volume (contrast) setting - fine tuning, original
  send_data(lcd, 0x03); // internal resistor ratio - coarse adjustment

  // set contrast (PHILLIPS)
  send_cmd(lcd, VOLCTR);
  send_data(lcd, 0x30);

  // nop (EPSON)
  send_cmd(lcd, NOP);

  // nopp (PHILLIPS)
  send_cmd(lcd, NOPP);

  usleep(200 * 1000);

  // display on (EPSON)
  send_cmd(lcd, DISON);

  // display on (PHILLIPS)
  send_cmd(lcd, DISPON);

  return 0;
}

void lcd_dispose(LCD *lcd) {
  close(lcd->fd);
}
