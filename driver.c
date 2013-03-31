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

#include "gpio.h"
#include "spi.h"
#include "driver.h"

static int lcd_reset(int reset_pin) {
  if (gpio_setup() != 0) {
    perror("Couldn't setup GPIO");
    exit(1);
  }

  gpio_set_output(reset_pin);

  int reset_pin_mask = 1 << reset_pin;
  int delay_between_commands = 200 * 1000;

  printf("Setting RESET low\n");
  gpio_clear(reset_pin_mask);

  usleep(delay_between_commands);

  printf("Setting RESET high\n");
  gpio_set(reset_pin_mask);

  usleep(delay_between_commands);

  return 0;
}

static void send(LCD *lcd, uint16_t word) {
  if (spi_send_word(lcd->fd, word) < 0) {
    perror("Error sending SPI message");
    exit(1);
  }
}

static void send_cmd(LCD *lcd, uint8_t cmd) {
  send(lcd, cmd);
}

static void send_data(LCD *lcd, uint8_t data) {
  send(lcd, (uint16_t) data | 0x100);
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


int lcd_init(LCD *lcd, char *dev, int reset_pin, int type) {
  int res = lcd_reset(reset_pin);
  if (res < 0) {
    perror("Failed to reset LCD.");
    return 1;
  }

  bzero(lcd, sizeof(LCD));
  lcd->dev = dev;
  lcd->type = type;
  lcd->fd = spi_init(dev);
  if (lcd->fd < 0)
    return lcd->fd;

  // NOP
  send_cmd(lcd, NOP);

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
