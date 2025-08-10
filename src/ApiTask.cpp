#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "ApiTask.h"
#include <.env.h>

std::string WEATHER_API_REQUEST = std::string{"http://api.open-meteo.com/v1/forecast?latitude="} + LAT + "&longitude=" + LON + "&current=temperature_2m,is_day,rain,weather_code,cloud_cover,wind_speed_10m&timezone=America%2FChicago&wind_speed_unit=mph&temperature_unit=fahrenheit&precipitation_unit=inch";
std::string NEWS_API_REQUEST = std::string{"http://newsapi.org/v2/top-headlines?sources=cbs-news,cnn,ars-technica,abc-news,the-washington-post,nbc-news,associated-press,bbc-news,bloomberg,usa-today,time,the-wall-street-journel,techradar,recode,politico,newsweek,msnbc,ign,hacker-news&pageSize=100&apiKey="} + NEWS_API_KEY;

void requestWeatherGet(HTTPClient& http, void (*callback)(JsonDocument)) {
    http.begin(WEATHER_API_REQUEST.c_str());

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

        callback(doc);
    }

    http.end();
}

void requestWeatherTask(void* parameters) {
    HTTPClient http;
    TaskParams taskParams = *(TaskParams*)parameters;
    for (;;) {
        vTaskSuspend(NULL);
        requestWeatherGet(http, taskParams.callback);
    }
}

// Non-RTOS version
void requestWeatherSync(void (*callback)(JsonDocument)) {
    HTTPClient http;
    requestWeatherGet(http, callback);
}

void requestNewsTask(void* parameters) {
    HTTPClient http;
    for (;;) {
        vTaskSuspend(NULL);
        http.begin(WEATHER_API_REQUEST.c_str());

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
                continue;
            }

        }
        http.end();
    }
}

