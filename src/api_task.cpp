#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <vector>

#include ".env.h"
#include "api_task.hpp"

const std::string WEATHER_API_REQUEST = std::string{"http://api.open-meteo.com/v1/forecast?latitude="} + LAT + "&longitude=" + LON + "&current=temperature_2m,is_day,rain,weather_code,cloud_cover,wind_speed_10m&timezone=America%2FChicago&wind_speed_unit=mph&temperature_unit=fahrenheit&precipitation_unit=inch";
const std::string NEWS_API_REQUEST = std::string{"http://newsapi.org/v2/top-headlines?sources=cbs-news,cnn,ars-technica,abc-news,the-washington-post,nbc-news,associated-press,bbc-news,bloomberg,usa-today,time,the-wall-street-journel,techradar,recode,politico,newsweek,msnbc,ign,hacker-news&pageSize=20&apiKey="} + NEWS_API_KEY;

void requestWeatherTask(void* parameters) {
    HTTPClient http;
    TaskParams taskParams = *(TaskParams*)parameters;

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
                return;
            }

            taskParams.callback(doc);
        }

        http.end();
    }
}

void requestNewsPage(HTTPClient& http, int page, std::vector<std::string>& newsTexts) {
    char endpoint[350] = "";
    sprintf(endpoint, "%s%s%d", NEWS_API_REQUEST.c_str(), "&page=", page);
    http.begin(endpoint);

    int httpResponseCode = http.GET();
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode == 200) {
        String payload = http.getString();

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
            Serial.print("deserializeJson() returned ");
            Serial.println(error.c_str());
            return;
        }

        JsonArray articles = doc["articles"];
        for (JsonVariant article : articles) {
            std::string title = article["title"];
            std::string desc = article["description"];
            newsTexts.push_back(title + "   " + desc);
        }
    }
    http.end();
}

void requestNewsTask(void* parameters) {
    HTTPClient http;
    NewsTaskParams taskParams = *(NewsTaskParams*)parameters;

    std::vector<std::string>& newsTexts = taskParams.newsTexts;

    for (;;) {
        vTaskSuspend(NULL);

        int hourDiff = taskParams.currHour - taskParams.prevHour;
        if (hourDiff < 0) {
            hourDiff += 24;
        }
        if (hourDiff == 3) { // Updates every 3 hours
            newsTexts.clear();
            http.begin(NEWS_API_REQUEST.c_str());

            int httpResponseCode = http.GET();
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);

            // Only advance if HTTP response is 200, otherwise skip and suspend
            if (httpResponseCode != 200) {
                continue;
            }

            String payload = http.getString();
            Serial.println(payload.substring(0, 100) + "...");
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (error) {
                Serial.print("deserializeJson() returned ");
                Serial.println(error.c_str());
                continue;
            }

            JsonArray articles = doc["articles"];
            for (JsonVariant article : articles) {
                std::string title = article["title"];
                std::string desc = article["description"];
                newsTexts.push_back(title + "   " + desc);
            }

            // Request more news articles based on how many results API says
            int totalResults = doc["totalResults"].as<int>();
            if (totalResults > 20) {
                for (int i = 1; i < (totalResults - 1) / 20 + 1; i++) {
                    requestNewsPage(http, i + 1, newsTexts);
                }
            }

            taskParams.newsTextsIdx = 0;
            taskParams.prevHour = taskParams.currHour;

            Serial.print("Number of articles: ");
            Serial.println(newsTexts.size());

            http.end();
        }

        // Time to call callback w/ appropriate news article
        taskParams.callback(newsTexts[taskParams.newsTextsIdx]);
        taskParams.newsTextsIdx = (taskParams.newsTextsIdx + 1) % newsTexts.size();

        Serial.print("taskParams idx: ");
        Serial.print(taskParams.newsTextsIdx);
        Serial.print(" / ");
        Serial.println(taskParams.newsTexts.size());
    }
}

