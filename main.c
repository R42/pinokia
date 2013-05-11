#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include "driver.h"

#include <math.h>

#define fracf(x) ((x) - floorf(x))

int main(int argc, char ** argv) {
  int res;

  int demo = argc == 1 ? 0 : atoi(argv[1]);

  LCD *lcd = (LCD *) malloc(sizeof(LCD));

  res = lcd_init(lcd, "/dev/spidev0.0", 25, TYPE_PHILIPS);
  if (res < 0) {
    perror("Failed to init LCD.");
    return 1;
  }

  lcd_clear(lcd, BLACK);

  const int w = 130, h = 130;

  if (demo == 0) {
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
  } else if (demo == 1) {
    int i;
    int offset;
    for (offset = 0; offset < w / 2; offset++) {
      for (i = w - (2 * offset); i > 0; i--) {
        lcd_set_pixel(lcd, i + offset, 1 + offset, RED  );
        lcd_set_pixel(lcd, w - offset, i + offset, GREEN);
        lcd_set_pixel(lcd, i + offset, h - offset, BLUE );
        lcd_set_pixel(lcd, 1 + offset, i + offset, WHITE);
      }
    }
  } else if (demo == 2) {
    lcd_clear(lcd, RED);
    sleep(2);
    lcd_clear(lcd, GREEN);
    sleep(2);
    lcd_clear(lcd, BLUE);
  } else if (demo == 3) {
    int i, s = 1, e = 130;
    for (i = s; i <= e; i++) {
      lcd_set_pixel(lcd, i, s, RED);
      lcd_set_pixel(lcd, i, e, RED);
      lcd_set_pixel(lcd, s, i, RED);
      lcd_set_pixel(lcd, e, i, RED);
    }
  } else {
    int i;
    for(;;) {
      for (i = 0; i < 0x1000; i++) {
        lcd_clear(lcd, i);
        usleep(100 * 1000);
      }
      for (i = 0xfff; i >= 0; i--) {
        lcd_clear(lcd, i);
        usleep(100 * 1000);
      }
    }
  }

  lcd_dispose(lcd);
  free(lcd);

  return 0;
}
