#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <utility>
#include <vector>

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include <Fonts/FreeSerifBold9pt7b.h>
#include <Fonts/TomThumb.h>
#include <Fonts/Picopixel.h>
// #include "Naxaci9pt7b.h"


#include ".env.h" // Defines WIFI_SSID, WIFI_PASSWORD, LAT, and LON
#include "DayNightIcons.h"
#include "ScrollingText.h"

MatrixPanel_I2S_DMA* dma_display = nullptr;
std::vector<ScrollingText> scrollingTexts;

// RGB565 colors
uint16_t WHITE = 65535;
uint16_t BLANK = 0;
uint16_t PURPLE = 41436;
uint16_t GREEN = 24230;
uint16_t BLUE = 9853;
uint16_t TEXTWHITE = 48667;
uint16_t ORANGE = 62914;
uint16_t YELLOW = 65477;
uint16_t GRAY = 27501;
uint16_t RED = 57826;
uint16_t STORM_BLUE = 624;
uint16_t SNOW_BLUE = 36314;
uint16_t RAIN_BLUE = 2525;
uint16_t PINK = 59667;

const char* NTP_SERVER = "pool.ntp.org";

const char* DAYS_OF_WEEK[] = {"Sun", "Mon", "Tues", "Wed", "Thurs", "Fri", "Sat"};
const char* MONTHS[] = {"Jan.", "Feb.", "Mar.", "Apr.", "May.", "Jun.", "Jul.", "Aug.", "Sep.", "Oct.", "Nov.", "Dec."};

std::string API_REQUEST = std::string{"http://api.open-meteo.com/v1/forecast?latitude="} + LAT + "&longitude=" + LON + "&current=temperature_2m,is_day,rain,weather_code,cloud_cover,wind_speed_10m&timezone=America%2FChicago&wind_speed_unit=mph&temperature_unit=fahrenheit&precipitation_unit=inch";

int prevMinute = 0;
int prevMinute15 = 0;

unsigned long prevUpdateMillis = 0;

HTTPClient http;

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

void drawXbm565(int x, int y, int width, int height, const char *xbm, uint16_t color = 0xffff) 
{
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

    Serial.print("Wday:");
    Serial.println(time.tm_wday);
    Serial.print("Mon:");
    Serial.println(time.tm_mon);
}

void updateWeather(JsonDocument doc) {
    JsonObject current = doc["current"];

    char temp[6] = "";
    sprintf(temp, "%.1f", current["temperature_2m"].as<double>());

    int isDay = current["is_day"];
    int wmoCode = current["weather_code"];

    const char* weatherDesc;
    int weatherColor;
    std::tie(weatherDesc, weatherColor) = wmoCodeToStr(wmoCode, isDay);

    dma_display->fillRect(0, 0, 64, 7, BLANK);

    dma_display->setFont(&TomThumb);
    dma_display->setCursor(0, 6);
    dma_display->setTextColor(weatherColor);
    dma_display->print(weatherDesc);

    dma_display->setCursor(32, 6);
    dma_display->setTextColor(PINK);
    dma_display->print(temp);
    dma_display->print("F");

    if (current["is_day"]) {
        drawXbm565(57, 0, 5, 7, sun_bits, YELLOW);
    } else {
        drawXbm565(57, 0, 5, 7, moon_bits, YELLOW);
    }
}
void scrollTextFinishCallback(ScrollingText& scrollingText) {
    scrollingTexts.erase(std::remove(scrollingTexts.begin(), scrollingTexts.end(), scrollingText), scrollingTexts.end());
}

void addScrollingText(const std::string& text, const GFXfont* font, int startX, int startY, uint16_t color, int animDelay = 40) {
    ScrollingText e = ScrollingText{dma_display, text, font, startX, startY, color, animDelay, scrollTextFinishCallback};
    scrollingTexts.push_back(e);
}

void requestWeatherTask(void* parameters) {
    http.begin(API_REQUEST.c_str());

    int httpResponseCode = http.GET();
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode == 200) {
        String payload = http.getString();
        Serial.println(payload);

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
            Serial.print("deserializeJson() returned ");
            Serial.println(error.c_str());
            vTaskDelete(NULL);
            return;
        }

        updateWeather(doc);
    }
    http.end();
    vTaskDelete(NULL);
}

// Non-RTOS version
void requestWeatherTask() {
    http.setReuse(false);
    http.begin(API_REQUEST.c_str());
    Serial.println(WiFi.status());

    int httpResponseCode = http.GET();
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode == 200) {
        String payload = http.getString();
        Serial.println(payload);

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
            Serial.print("deserializeJson() returned ");
            Serial.println(error.c_str());
            return;
        }

        updateWeather(doc);
    }
    http.end();
}

void updateInfo() {
    tm time;
    if (!getLocalTime(&time)) {
        Serial.println("Failed to obtain time");
        return;
    }

    if (prevMinute != time.tm_min) {
        // Time to update HH:MM part
        prevMinute = time.tm_min;

        dma_display->fillRect(0, 9, 42, 14, BLANK);

        dma_display->setFont(&FreeSerifBold9pt7b);
        dma_display->setTextColor(ORANGE);
        dma_display->setCursor(0, 22);
        dma_display->print(&time, "%I");
        dma_display->print(':');
        dma_display->print(&time, "%M");
    }

    int minDiff = time.tm_min - prevMinute15;
    if (minDiff < 0) {
        minDiff += 60;
    }
    if (minDiff == 15) { // Been 15 minutes 
        Serial.println("Requesting weather");
        xTaskCreatePinnedToCore(
            requestWeatherTask,
            "Weather HTTP",
            8192,
            NULL,
            1,
            NULL,
            1
        );
        prevMinute15 = time.tm_min;
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

    if (time.tm_hour == 0) {
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

    prevMinute15 = time.tm_min;

    updateDate(time);
    http.setReuse(false);
    requestWeatherTask(); // non-RTOS

    // addScrollingText("The quick brown fox jumps over the lazy dog because why not", &TomThumb, 64, 6, PURPLE);
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
