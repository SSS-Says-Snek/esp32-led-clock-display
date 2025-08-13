#pragma once

#include <string>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

class ScrollingText {
    std::string text{};
    const GFXfont* font{nullptr};
    MatrixPanel_I2S_DMA* dma_display{nullptr};

    int textXPos{};
    int textYPos{};
    int animDelay{};

    bool stopped{false};

    unsigned long prevAnimMillis{};

    int16_t x1{};
    int16_t y1{};

    uint16_t width{};
    uint16_t height{};

    uint16_t textColor{};

    void (*finishCallback)(ScrollingText&);

public:
    ScrollingText(MatrixPanel_I2S_DMA* dma_display, std::string text, const GFXfont* font, int textXStart, int textYStart, uint16_t textColor, int animDelay, void (*cb)(ScrollingText&));
    bool operator==(const ScrollingText& rhs);
    void update();
    void stop();
};