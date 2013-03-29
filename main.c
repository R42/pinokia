#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include "modules.h"
#include "driver.h"

static int writeToFile(char *filename, char *what) {
  FILE *file;

  file = fopen(filename, "w");
  if (!file) return -1;

  fwrite(what, strlen(what), 1, file);

  return fclose(file);
}

static int resetLcd() {
  int res;
  int delay_between_commands = 200 * 1000;

  printf("Exporting gpio25\n");
  res = writeToFile("/sys/class/gpio/export", "25");
  if (res)
    return res;

  usleep(delay_between_commands);

  printf("Setting gpio25 to output\n");
  res = writeToFile("/sys/class/gpio/gpio25/direction", "out");
  if (res)
    return res;

  usleep(delay_between_commands);

  printf("Setting gpio25 to 0\n");
  res = writeToFile("/sys/class/gpio/gpio25/value", "0");
  if (res)
    return res;

  usleep(delay_between_commands);

  printf("Setting gpio25 to 1\n");
  res = writeToFile("/sys/class/gpio/gpio25/value", "1");
  if (res)
    return res;

  usleep(delay_between_commands);

  printf("Unexporting gpio25\n");
  res = writeToFile("/sys/class/gpio/unexport", "25");
  if (res)
    return res;

  return 0;
}


int main(int argc, char * argv[]) {
  int res;
  LCD lcd;

  // make sure SPI kernel modules are loaded
  loadSpiModules();

  res = resetLcd();
  if (res < 0) {
    perror("Failed to reset LCD.");
    return 1;
  }

  printf("Init'ing\n");
  res = lcd_init(&lcd, "/dev/spidev0.0", TYPE_EPSON);
  if (res < 0) {
    perror("Failed to init LCD.");
    return 1;
  }

  printf("Ready. Press [return] to clear the screen.\n");
  getc(stdin);

  printf("Setting the screen green\n");
  res = lcd_set_pixel(&lcd, 10, 10, GREEN);
  if (res < 0) {
    perror("Failed to clear LCD.");
    return 1;
  }

  printf("Ready. Press [return] to exit.\n");
  getc(stdin);
  // sleep(9);

  lcd_dispose(&lcd);

  return 0;
}
