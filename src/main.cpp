#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include <utility>
#include <vector>

#include <Fonts/FreeSerifBold9pt7b.h>
#include <Fonts/TomThumb.h>
#include <Fonts/Picopixel.h>
// #include "Naxaci9pt7b.h"


#include ".env.h" // Defines WIFI_SSID, WIFI_PASSWORD, LAT, LON, and NEWS_API_KEY
#include "day_night_icons.hpp"
#include "scrolling_text.hpp"
#include "api_task.hpp"
#include "utils.hpp"
#include "constants.hpp"

MatrixPanel_I2S_DMA* dma_display = nullptr;
std::vector<ScrollingText> scrollingTexts;

const char* NTP_SERVER = "pool.ntp.org";

const char* DAYS_OF_WEEK[] = {"Sun", "Mon", "Tues", "Wed", "Thurs", "Fri", "Sat"};
const char* MONTHS[] = {"Jan.", "Feb.", "Mar.", "Apr.", "May.", "Jun.", "Jul.", "Aug.", "Sep.", "Oct.", "Nov.", "Dec."};

int prevMinute = 0;
int prevMinute2 = 0;
int prevMinute10 = 0;
TaskHandle_t weatherTaskHandle;
TaskHandle_t newsTaskHandle;

unsigned long prevUpdateMillis = 0;

enum class TopBarStatus {
    WEATHER,
    NEWS
};
TopBarStatus currentTopStatus = TopBarStatus::WEATHER;

TaskParams weatherTask;
NewsTaskParams newsTask;

void setupMatrix() {
    // LED Matrix setup
    HUB75_I2S_CFG mxconfig;
    mxconfig.mx_height = 32;
    mxconfig.chain_length = 1;
    mxconfig.gpio.e = -1;
    mxconfig.driver = HUB75_I2S_CFG::FM6126A;
    mxconfig.clkphase = false;

    // Initialize DMA display
    dma_display = new MatrixPanel_I2S_DMA(mxconfig);
    // dma_display->setBrightness8(5);

    dma_display->setTextWrap(false);

    if (not dma_display->begin())
        Serial.println("I2S memory allocation failed");
}

void setupWifi() {
    Serial.println(WIFI_SSID);
    Serial.println(WIFI_PASSWORD);
    // WiFi.config(IPAddress{192, 168, 1, 140}, IPAddress{192, 168, 1, 1}, IPAddress{255, 255, 255, 0}, IPAddress{192, 168, 1, 110}, IPAddress{8, 8, 4, 4});
    WiFi.setHostname("ESP32-Clock");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    dma_display->setCursor(0, 0);
    dma_display->setTextColor(WHITE);

    while (WiFi.status() != WL_CONNECTED) {
        delay(300);
        dma_display->print('.');
    }

    Serial.println(WiFi.localIP());
    dma_display->clearScreen();
    dma_display->setCursor(0, 0);
    dma_display->print("Conn. WiFi");

    configTime(-5*3600, 0, NTP_SERVER);

    delay(1000);
}

void updateDate(tm time) {
    dma_display->fillRect(0, 25, 64, 7, BLANK);
    dma_display->drawFastVLine(20, 25, 7, TEXTWHITE);
    dma_display->drawFastVLine(45, 25, 7, TEXTWHITE);

    // Text
    dma_display->setFont(&TomThumb);

    // Day of Week
    dma_display->setCursor(0, 31);
    dma_display->setTextColor(PURPLE);
    dma_display->print(DAYS_OF_WEEK[time.tm_wday]);

    // Month-Day
    dma_display->setCursor(22, 31);
    dma_display->setTextColor(GREEN);
    dma_display->print(MONTHS[time.tm_mon]);
    dma_display->print(' ');
    dma_display->print(time.tm_mday);

    dma_display->setCursor(47, 31);
    dma_display->setTextColor(BLUE);
    dma_display->print(time.tm_year + 1900); // POSIX says tm_year's relative to 1900
}

void updateWeather(JsonDocument doc) {
    Serial.println("Updating weather");
    if (currentTopStatus == TopBarStatus::NEWS) {
        currentTopStatus == TopBarStatus::WEATHER;
        return; // Update after
    }
    currentTopStatus == TopBarStatus::WEATHER;

    JsonObject current = doc["current"];

    char temp[6] = "";
    sprintf(temp, "%.1f", current["temperature_2m"].as<double>());

    int isDay = current["is_day"];
    int wmoCode = current["weather_code"];

    const char* weatherDesc;
    int weatherColor;
    std::tie(weatherDesc, weatherColor) = wmoCodeToStr(wmoCode, isDay);

    dma_display->fillRect(0, 0, 64, 8, BLANK);

    dma_display->setFont(&TomThumb);
    dma_display->setCursor(0, 6);
    dma_display->setTextColor(weatherColor);
    dma_display->print(weatherDesc);

    dma_display->setCursor(32, 6);
    dma_display->setTextColor(PINK);
    dma_display->print(temp);
    dma_display->print("F");

    if (current["is_day"]) {
        drawXbm565(dma_display, 57, 0, 5, 7, sun_bits, YELLOW);
        dma_display->setBrightness(128);
    } else {
        drawXbm565(dma_display, 57, 0, 5, 7, moon_bits, YELLOW);
        dma_display->setBrightness8(15);
    }
}

void scrollTextFinishCallback(ScrollingText& scrollingText) {
    for (int i = 0; i < scrollingTexts.size(); i++) {
        if (scrollingTexts[i] == scrollingText) {
            scrollingText.stop();
            Serial.println("Stopped");
        }
    }
    scrollingTexts.erase(std::remove(scrollingTexts.begin(), scrollingTexts.end(), scrollingText), scrollingTexts.end());

    dma_display->fillRect(0, 0, 64, 8, BLANK);
    delay(400);

    currentTopStatus = TopBarStatus::WEATHER;

    dma_display->fillRect(0, 0, 64, 8, BLANK);
    // updateWeather(weatherDoc); Doesn't work for some reason wtf
    vTaskResume(weatherTaskHandle);
}

void addScrollingText(const std::string& text, const GFXfont* font, int startX, int startY, uint16_t color, int animDelay = 40) {
    ScrollingText e = ScrollingText{dma_display, text, font, startX, startY, color, animDelay, scrollTextFinishCallback};
    scrollingTexts.push_back(e);
}

void updateNews(const std::string& news) {
    Serial.println(news.c_str());
    
    if (currentTopStatus != TopBarStatus::NEWS) {
        dma_display->fillRect(0, 0, 64, 7, BLANK);
        addScrollingText(news, &TomThumb, 64, 6, PURPLE, 70);
    }

    currentTopStatus = TopBarStatus::NEWS;
}

void updateInfo() {
    tm time;
    if (!getLocalTime(&time)) {
        Serial.println("Failed to obtain time");
        return;
    }

    // Every minute
    if (prevMinute != time.tm_min) {
        // Time to update HH:MM part
        prevMinute = time.tm_min;

        dma_display->fillRect(0, 9, 43, 14, BLANK);

        dma_display->setFont(&FreeSerifBold9pt7b);
        dma_display->setTextColor(ORANGE);
        dma_display->setCursor(0, 22);
        dma_display->print(&time, "%I");
        dma_display->print(':');
        dma_display->print(&time, "%M");

        dma_display->drawFastHLine(0, 24, 64, TEXTWHITE);
        dma_display->drawFastHLine(0, 8, 64, TEXTWHITE);

        newsTask.currHour = time.tm_hour;
    }


    // Every 2 minutes
    int minDiff = time.tm_min - prevMinute2;
    if (minDiff < 0) {
        minDiff += 60;
    }
    if (minDiff == 2) {
        Serial.println("Requesting weather");
        vTaskResume(weatherTaskHandle);
        prevMinute2 = time.tm_min;
    }

    // Every 10 minutes
    minDiff = time.tm_min - prevMinute10;
    if (minDiff < 0) {
        minDiff += 60;
    }
    if (minDiff == 3) {
        Serial.println("Requesting news");
        vTaskResume(newsTaskHandle);
        prevMinute10 = time.tm_min;
    }

    dma_display->fillRect(43, 9, 21, 14, BLANK);

    dma_display->setFont();
    dma_display->setTextColor(RED);
    dma_display->setCursor(41, 10);
    dma_display->print(':');
    dma_display->print(&time, "%S");

    dma_display->setFont(&Picopixel);
    dma_display->setCursor(47, 22);

    if (time.tm_hour >= 12) {
        dma_display->print("P.M");
    } else {
        dma_display->print("A.M");
    }

    if (time.tm_hour == 0 && time.tm_min == 0) {
        updateDate(time);
    }
}

void setup() {
    Serial.begin(115200);

    setupMatrix();
    setupWifi();

    // Testing out the display
    dma_display->fillScreenRGB888(255, 255, 255);
    delay(500);
    dma_display->fillScreenRGB888(255, 0, 0);
    delay(500);
    dma_display->fillScreenRGB888(0, 255, 0);
    delay(500);
    dma_display->fillScreenRGB888(0, 0, 255);
    delay(500);
    dma_display->fillScreenRGB888(255, 255, 255);

    dma_display->clearScreen();
    delay(1000);

    // Draw lines
    dma_display->drawFastVLine(20, 25, 7, TEXTWHITE);
    dma_display->drawFastVLine(45, 25, 7, TEXTWHITE);
    dma_display->drawFastHLine(0, 24, 64, TEXTWHITE);
    dma_display->drawFastHLine(0, 8, 64, TEXTWHITE);

    tm time;
    if (!getLocalTime(&time)) {
        Serial.println("Failed to obtain time");
        return;
    }

    prevMinute2 = time.tm_min;
    prevMinute10 = time.tm_min;

    // Create Weather Task
    weatherTask = { .callback = updateWeather };
    xTaskCreatePinnedToCore(
        requestWeatherTask,
        "Weather HTTP",
        8192,
        &weatherTask,
        1,
        &weatherTaskHandle,
        1
    );

    // Force the news task to update, since it's "been" 3 hours
    int newsPrevHour = time.tm_hour - 3;
    if (newsPrevHour < 0) {
        newsPrevHour += 24;
    }
    
    newsTask = { .newsTexts = std::vector<std::string>{}, .newsTextsIdx = 0, .prevHour = newsPrevHour, .currHour = time.tm_hour, .callback = updateNews };
    xTaskCreatePinnedToCore(
        requestNewsTask,
        "News HTTP",
        8192,
        &newsTask,
        1,
        &newsTaskHandle,
        1
    );
    updateDate(time);

    vTaskResume(weatherTaskHandle);
    vTaskResume(newsTaskHandle);
}

void loop() {
    unsigned long now = millis();
    if (now > prevUpdateMillis + 200) {
        prevUpdateMillis = now;
        updateInfo();
    }

    for (ScrollingText& scrollingText : scrollingTexts) {
        scrollingText.update();
    }
}
