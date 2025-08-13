#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

struct TaskParams {
    void (*callback)(JsonDocument);
};

struct NewsTaskParams {
    std::vector<std::string> newsTexts;
    int newsTextsIdx;
    int prevHour;
    int currHour;
    void (*callback)(const std::string&);
};

void requestWeatherTask(void* parameters);
void requestWeatherSync(void (*callback)(JsonDocument));
void requestNewsTask(void* parameters);