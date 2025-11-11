<p align="center">
  <img src="assets/image.jpg" width=1000>
</p>

<h1 align="center">ESP32 LED Clock Display w/ Weather and News</h1>
<p align="center">A programmed 64x32 LED Matrix board, made with C++</p>

<p align="center">
    <img alt="Static Badge" src="https://img.shields.io/badge/C%2B%2B17-3776ab?style=for-the-badge&logo=c%2B%2B&logoColor=ffffff">
    <img alt="GitHub License" src="https://img.shields.io/github/license/SSS-Says-Snek/esp32-led-clock-display?style=for-the-badge">
    <img alt="GitHub repo size" src="https://img.shields.io/github/repo-size/SSS-Says-Snek/esp32-led-clock-display?style=for-the-badge">
    <img alt="GitHub last commit" src="https://img.shields.io/github/last-commit/SSS-Says-Snek/esp32-led-clock-display?style=for-the-badge">
</p>

# Features
- Date
- Time (including seconds)
- Weather status, e.g Sunny, Raining, Hailstorming
- Temperature outside, updated in 15 minute intervals
- Daily news that scrolls across the top bar

# Use

Make sure to define a `.env.h` in the `src/` directory with the following contents:
```c++
#pragma once

#define WIFI_SSID "your SSID"
#define WIFI_PASSWORD "your password"

#define LAT "your latitude"
#define LON "your longitude"
#define NEWS_API_KEY "Your News API key from newsapi.org"
```

# Install

Then, flash it to your ESP32 using your favorite manager
