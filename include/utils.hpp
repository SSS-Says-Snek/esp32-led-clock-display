#pragma once

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#include <utility>

std::pair<const char*, int> wmoCodeToStr(int wmoCode, int isDay);
void drawXbm565(MatrixPanel_I2S_DMA* dma_display, int x, int y, int width, int height, const char *xbm, uint16_t color = 0xffff);