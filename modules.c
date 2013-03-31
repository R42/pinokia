#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static int moduleLoaded(char *modName) {
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

  if (moduleLoaded(module)) {
    return 0;
  }

  printf("Loading module: %s", module);
  sprintf(cmd, "modprobe %s", module);
  system(cmd);

  sleep(1);

  if (!moduleLoaded(module)) {
    fprintf(stderr, "Unable to load %s\n", module);
    exit(1);
  }
}

void loadSpiModules() {
  loadModule("spidev");
  loadModule("spi_bcm2708");
}
