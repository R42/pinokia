
#include "modules.h"
#include "spi.h"

const char* SPI_DEV = "/dev/spidev0.0";

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
    if (send_word(fd, 0xAA28) < 0) {
      perror("Error sending SPI message");
      exit(1);
    }
  }
  sleep(1);
  printf("Sent.\n");

  close(fd);

  return 0;
}
