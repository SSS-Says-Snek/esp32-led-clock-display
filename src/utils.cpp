#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#include <utility>

#include <constants.hpp>

std::pair<const char*, int> wmoCodeToStr(int wmoCode, int isDay) {
    switch (wmoCode) {
        case 0:
        case 1:
            if (isDay) {
                return std::make_pair("Sunny", YELLOW);
            }
            return std::make_pair("Clear", YELLOW);
        case 2:
        case 3:
            return std::make_pair("Cloudy", GRAY);
        case 45:
        case 48:
            return std::make_pair("Foggy", GRAY);
        case 51:
        case 53:
            return std::make_pair("Drizzle", BLUE);
        case 55:
            return std::make_pair("Drizzle", STORM_BLUE);

        case 61:
        case 64:
            return std::make_pair("Rainy", RAIN_BLUE);
        case 65:
            return std::make_pair("Rainy", STORM_BLUE);

        case 66:
        case 67:
            return std::make_pair("Fz.Rain", SNOW_BLUE);

        case 71:
        case 73:
        case 75:
        case 77:
        case 85:
        case 86:
            return std::make_pair("Snowy", SNOW_BLUE);

        case 80:
        case 81:
            return std::make_pair("Showers", RAIN_BLUE);
        case 82:
            return std::make_pair("Showers", STORM_BLUE);

        case 95:
        case 96:
        case 99:
            return std::make_pair("Thunder", STORM_BLUE);

        default:
            return std::make_pair("?????", PURPLE);
    }
}

void drawXbm565(MatrixPanel_I2S_DMA* dma_display, int x, int y, int width, int height, const char *xbm, uint16_t color = 0xffff) {
  if (width % 8 != 0) {
      width =  ((width / 8) + 1) * 8;
  }
    for (int i = 0; i < width * height / 8; i++ ) {
      unsigned char charColumn = pgm_read_byte(xbm + i);
      for (int j = 0; j < 8; j++) {
        int targetX = (i * 8 + j) % width + x;
        int targetY = (8 * i / (width)) + y;
        if (bitRead(charColumn, j)) {
          dma_display->drawPixel(targetX, targetY, color);
        }
      }
    }
}
