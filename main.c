#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include "driver.h"

#include <math.h>

#define fracf(x) ((x) - floorf(x))

int main(int argc, char * argv[]) {
  int res;
  LCD *lcd = (LCD *) malloc(sizeof(LCD));

  res = lcd_init(lcd, "/dev/spidev0.0", 25, TYPE_PHILIPS);
  if (res < 0) {
    perror("Failed to init LCD.");
    return 1;
  }

  lcd_clear(lcd, BLACK);

  if (argc > 0 && strcmp("0", argv[1]) == 0) {
    const int w = 130, h = 130;
    uint32_t x, y, color;
    float r, g, b, s2 = sqrtf(2), dd = w * s2;
    for (x = 0; x < w; x++) {
      for (y = 0; y < h; y++) {
        r = (dd - sqrtf(x * x + y * y            )) * 0xf / dd; if (r < 0) r = 0;
        g = (dd - sqrtf((w - x) * (w - x) + y * y)) * 0xf / dd; if (g < 0) g = 0;
        b = (dd - sqrtf(x * x + (h - y) * (h - y))) * 0xf / dd; if (b < 0) b = 0;

        color = RGB((int) r + (int) (1.0f * rand() / RAND_MAX + fracf(r)),
                    (int) g + (int) (1.0f * rand() / RAND_MAX + fracf(g)),
                    (int) b + (int) (1.0f * rand() / RAND_MAX + fracf(b)));

        lcd_set_pixel(lcd, x + 1, y + 1, color);
      }
    }
  } else {
    int i;
    int offset;
    for (offset = 0; offset < 130 / 2; offset++) {
      for (i = 130 - (2 * offset); i > 0; i--) {
        lcd_set_pixel(lcd,   i + offset,   1 + offset, RED  );
        lcd_set_pixel(lcd, 130 - offset,   i + offset, GREEN);
        lcd_set_pixel(lcd,   i + offset, 130 - offset, BLUE );
        lcd_set_pixel(lcd,   1 + offset,   i + offset, WHITE);
      }
    }
  }

  lcd_dispose(lcd);
  free(lcd);

  return 0;
}
