#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

struct TaskParams {
    void (*callback)(JsonDocument);
};
void requestWeatherGet(HTTPClient& http, void* parameters);
void requestWeatherTask(void* parameters);
void requestWeatherSync(void (*callback)(JsonDocument));
void requestNewsTask(void* parameters);