# ESP WeatherClock NTP Firmware

This firmware transforms a low-cost ESP-based weather kit into a full Internet-synchronized meteorological station.  
It supports **ESP01, ESP8266, and ESP32** modules, along with **OLED displays (0.6" or 0.96")** using the SSD1306 driver.

## Features
- Synchronizes local time with any NTP server.
- Automatic summer/winter time adjustment.
- Real-time weather data from **OpenWeatherMap**.
- Web interface for configuration (WiFi credentials, timezone, API key, city ID).
- Data refresh and device reboot directly from the web interface.
- Compatible with Arduino IDE and PlatformIO.

## Hardware Requirements
- ESP01 / ESP8266 / ESP32 module
- OLED display (128x64 or 64x48)
- 3.3V power supply
- Optional USB programmer for ESP01

## Compilation
Use the Arduino IDE:
1. Install ESP8266 and ESP32 board packages.
2. Install the Adafruit SSD1306 and Adafruit GFX libraries.
3. Define the display resolution in `SSD1306.h` before compiling.
4. Flash the firmware using the correct board selection.

## Credits
Developed by J-RPM.  

YouTube: 
https://youtu.be/hKXPpA2Ls1I

Website:
https://wp.me/paKnHQ-Y2
