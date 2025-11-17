# ESP WeatherClock NTP Firmware — v1.15

This firmware transforms a low-cost ESP-based weather kit into a full Internet-synchronized meteorological station.  
It supports **ESP01, ESP8266, and ESP32** modules, along with **OLED displays (0.6" or 0.96")** using the SSD1306 driver.

Version **v1.14** introduces major improvements in memory usage, weather forecasting, and solar event calculations.

Version **v1.15** 
1 - It initially displays the date and time, waiting until it synchronizes with the NTP server.
2 - Management and organization are improved by displaying data on the OLED display.

---

## ✔️ New since v1.14
- Added **real-time weather forecast** extracted directly from OpenWeatherMap:
  - 3 upcoming forecast segments (3h, 6h, 9h)
  - Automatic interpretation of all official OWM weather icons (day/night variants)
- Added **sunrise and sunset times** (UTC → local conversion)
- Added **calculation of total daylight hours** for the current day
- Optimized JSON parsing using **streaming processing**, required to run on the ESP8266 due to large API responses (>22,000 chars)
- Improved memory efficiency:  
  - Up to **14 redesigned parsing routines** to maintain compatibility with ESP8266  
  - Faster and more stable updates with lower RAM usage
- Improved web interface:
  - Displays current weather, forecast segments, sunrise/sunset, and daylight duration
  - Includes refresh button (data refresh without forcing new API requests)
- More accurate NTP synchronization and timezone handling
- Improved overall stability and error handling

---

## Features
- Synchronizes local time with any NTP server.
- Automatic summer/winter (DST) time adjustment.
- Real-time weather data from **OpenWeatherMap**.
- Web interface for configuration:
  - WiFi credentials  
  - Timezone  
  - API key  
  - City ID  
- Local web panel for refresh and device reboot.
- Efficient API usage:
  - Current weather: every 10 minutes
  - Forecast refresh: every 2 hours
- Compatible with Arduino IDE and PlatformIO.

---

## Hardware Requirements
- ESP01 / ESP8266 / ESP32 module  
- OLED display (128x64 or 64x48)  
- 3.3V power supply  
- Optional USB programmer for ESP01  

---

## Compilation
Use the Arduino IDE:
1. Install ESP8266 and ESP32 board packages.
2. Install the Adafruit SSD1306 and Adafruit GFX libraries.
3. Define the display resolution in `SSD1306.h` before compiling.
4. Flash the firmware using the correct board selection.

---

## Credits
Developed by J-RPM.  
Version: **v1.15**

YouTube:  
https://youtu.be/hKXPpA2Ls1I
https://youtu.be/UKzfTVaWAdQ

Website:  
https://wp.me/paKnHQ-Y2
https://wp.me/paKnHQ-Yi
