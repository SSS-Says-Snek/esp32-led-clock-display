#include "ScrollingText.h"

ScrollingText::ScrollingText(MatrixPanel_I2S_DMA* dma_display, std::string text, const GFXfont* font, int textXStart, int textYStart, uint16_t textColor, int animDelay, void (*cb)(ScrollingText&))
    : dma_display(dma_display), text(text), font(font), textXPos(textXStart), textYPos(textYStart), textColor(textColor), animDelay(animDelay), finishCallback(cb) {
    }

bool ScrollingText::operator==(const ScrollingText& rhs) {
    return text == rhs.text && font == rhs.font && textXPos == rhs.textXPos && textYPos == rhs.textYPos;
}

void ScrollingText::update() {
    unsigned long now = millis();
    if (now > prevAnimMillis + animDelay && !stopped) {
        prevAnimMillis = now;
    
        textXPos -= 1;
        dma_display->setFont(font);
        dma_display->getTextBounds(text.c_str(), textXPos, textYPos, &x1, &y1, &width, &height);
        if (textXPos + width == 0) {
            // Do something, finished scrolling
            if (finishCallback) {
                finishCallback(*this);
                return; //aaa
            }
        }

        dma_display->fillRect(x1, y1, width + 64, height, 0);
        dma_display->setCursor(textXPos, textYPos);
        // dma_display->setCursor(textXPos, 6); // Why does the news always go to the bottom wtf
        dma_display->setTextColor(textColor);
        dma_display->print(text.c_str());
    }
}

void ScrollingText::stop() {
    stopped = true;
}