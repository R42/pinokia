#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

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

  printf("Exporting gpio25\n");
  res = writeToFile("/sys/class/gpio/export", "25");
  if (res)
    return res;

  usleep(200);

  printf("Setting gpio25 to output\n");
  res = writeToFile("/sys/class/gpio/gpio25/direction", "out");
  if (res)
    return res;

  usleep(200);

  printf("Setting gpio25 to 0\n");
  res = writeToFile("/sys/class/gpio/gpio25/value", "0");
  if (res)
    return res;

  usleep(200);

  printf("Setting gpio25 to 1\n");
  res = writeToFile("/sys/class/gpio/gpio25/value", "1");
  if (res)
    return res;

  usleep(200);

  printf("Unexporting gpio25\n");
  res = writeToFile("/sys/class/gpio/unexport", "25");
  if (res)
    return res;

  return 0;
}

static int moduleLoaded(char *modName)
{
  int len   = strlen(modName);
  int found = false;
  FILE *fd = fopen("/proc/modules", "r");
  char line [80];

  if(fd == NULL) {
    fprintf(stderr, "Unable to check modules: %s\n", strerror(errno));
    exit(1);
  }

  while(fgets(line, 80, fd) != NULL) {
    if(strncmp(line, modName, len) != 0)
      continue;

    found = true;
    break;
  }

  fclose(fd);

  return found;
}

static void loadModule(char * module) {
  char cmd[80];

  if(!moduleLoaded(module))
  {
    sprintf(cmd, "modprobe %s", module);
    system(cmd);
  }

  if(!moduleLoaded(module))
  {
    fprintf(stderr, "Unable to load %s\n", module);
    exit(1);
  }

}

static void loadSpiModules()
{
  loadModule("spidev");
  loadModule("spi_bcm2708");
  sleep(1);
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

  printf("Setting the screen green\n");
  res = lcd_clear(&lcd, GREEN);
  if (res < 0) {
    perror("Failed to clear LCD.");
    return 1;
  }

  printf("Ready\n");
  sleep(9);

  lcd_dispose(&lcd);

  return 0;
}
