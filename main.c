#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include "modules.h"
#include "driver.h"

int main(int argc, char * argv[]) {
  int res;
  LCD lcd;

  // make sure SPI kernel modules are loaded
  // loadSpiModules();

  printf("Init'ing\n");
  res = lcd_init(&lcd, "/dev/spidev0.0", 25, TYPE_EPSON);
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
