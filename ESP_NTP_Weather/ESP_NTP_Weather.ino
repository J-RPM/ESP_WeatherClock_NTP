/*
                             ***** ESP_NTP_Weather ******
                             * Compatible ESP32/ESP8266 *
                             ****************************
       (v1.14) >>> Muestra la Fecha, Hora y los datos meteorológicos
                               Copyright: J_RPM 
                               http://j-rpm.com/
                        https://www.youtube.com/c/JRPM

                        >>> API Key de OpenWeatherMap <<<
              Regístrate gratis en: https://openweathermap.org/api

                              >>> HARDWARE <<<
          
          Kit de reloj Digital DIY ESP8266 MINI reloj meteorológico WIFI
              https://es.aliexpress.com/item/1005008333782531.html
                              
                  LIVE D1 mini ESP32 ESP-32 WiFi + Bluetooth
                https://es.aliexpress.com/item/32816065152.html
                  
                ESP8266 ESP-12 ESP-12F CH340G CH340 V2 USB WeMos D1 Mini 
                  https://es.aliexpress.com/item/32651747570.html

                ESP8266 módulo de placa de desarrollo Wifi (ESP-01)
                https://es.aliexpress.com/item/1005006227695284.html

            HW-699 0.66 inch OLED display module, for D1 Mini (64x48)
                https://es.aliexpress.com/item/32868274524.html

         OLED de 0,96 pulgadas I2C IIC 128x64 SS - D - 1306 3,3 V-5 V
              https://es.aliexpress.com/item/1005006700828250.html

                          | Pin OLED | Pin ESP8266 | ESP-01S |
                          |--------- |-------------|---------|
                          | SDA | → | D2 (GPIO4)  | GPIO0   |
                          | SCL | → | D1 (GPIO5)  | GPIO2   |
                          | VCC | → | 3V3 o 5V    | 3V3     |
                          | GND | → | GND         | GND     |

        | Diferencia    | ESP32               | ESP8266                    | ESP-01S
        | ------------- | ------------------- | -------------------------- |-------------------------- |
        | Librería WiFi | #include <WiFi.h>   | #include <ESP8266WiFi.h>   | #include <ESP8266WiFi.h>  |
        | SDA           | GPIO21              | D2 / GPIO4                 | GPIO0                     |
        | SCL           | GPIO22              | D1 / GPIO5                 | GPIO2                     |
        | Wire.begin()  | Wire.begin(21,22)   | Wire.begin(D2, D1)         | Wire.begin(0, 2)          |

        | OLED Pin | Función      | ESP8266 Pin      |  ESP-01S      |
        | -------- | ------------ | ---------------- | ------------- |
        | **VCC**  | Alimentación | **3V3** o **5V** | **3V3**       |
        | **GND**  | Tierra       | **GND**          | **GND**       |
        | **SCL**  | Reloj I²C    | **D1 (GPIO5)**   | GPIO2         |
        | **SDA**  | Datos I²C    | **D2 (GPIO4)**   | GPIO0         |

            
                           >>> IDE Arduino <<<
 WEMOS MINI D1 ESP32 / ESP8266-LOLIN(WEMOS)D1 mini(clone) / Generic ESP8266 (ESP-01S)
       Add URLs: https://dl.espressif.com/dl/package_esp32_index.json
        https://arduino.esp8266.com/stable/package_esp8266com_index.json
                     https://github.com/espressif/arduino-esp32

                 ***************************************************
                 *  Compatibilidad universal ESP32 / ESP8266       * 
                 *  WiFi, WebServer, HTTPClient, NTP, JSON, etc.   *
                 ***************************************************
 
    | Funcionalidad | ESP32                                | ESP8266                                            |
    | ------------- | ------------------------------------ | -------------------------------------------------- |
    | WiFi          | <WiFi.h>                             | <ESP8266WiFi.h>                                    |
    | Web server    | <WebServer.h> + WebServer server(80) | <ESP8266WebServer.h> + ESP8266WebServer server(80) |
    | HTTP client   | <HTTPClient.h> + http.begin("url")   | <ESP8266HTTPClient.h> + http.begin(client, "url")  |
    | WiFiManager   | Misma librería                       | Misma librería                                     |
    | NTP / JSON    | Igual                                | Igual                                              |


 */
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// --- Detección automática de placa ---
#ifdef ESP32
  #include <WiFi.h>
  #include <WebServer.h>
  #include <HTTPClient.h>
  WebServer server(80);     // Servidor web en puerto 80
  HTTPClient http;          // Cliente HTTP
#else
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266HTTPClient.h>
  #include <WiFiClient.h>
  ESP8266WebServer server(80); // Servidor web en puerto 80
  HTTPClient http;             // Cliente HTTP
#endif

// --- Librerías comunes a ambas plataformas ---
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <math.h>
#include <time.h>
#include <DNSServer.h>
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
//#include <regex>

/////////////////////////////////////////////////////////////////
#include <EEPROM.h>
#define CONFIG_VERSION 0x0002  // Incrementar cuando cambie la estructura

#define EEPROM_SIZE        512   

#define API_KEY_ADDR       0
#define API_KEY_LEN        80

#define CITY_ID_ADDR       (API_KEY_ADDR + API_KEY_LEN)
#define CITY_ID_LEN        40

#define ZONE2_ADDR         (CITY_ID_ADDR + CITY_ID_LEN)
#define ZONE2_LEN          20

#define USE_ZONE2_ADDR     (ZONE2_ADDR + ZONE2_LEN)
#define USE_ZONE2_LEN      1

/////////////////////////////////
// Total usado: 0 → 141 bytes //
/////////////////////////////////

// ===================== CONFIGURACIÓN =====================
static String HWversion = "(v1.14)";
bool Test = false;  // Test de los iconos del tiempo en OLED
String CurrentTime, CurrentDate, webpage = "";

String apiKey = "";
String cityID = "";
static String zone1 = "SPAIN";
String zone2  = "UTC+0";
bool   T_Zone2 = false;
float temp, feels, humidity, pressure, wind;
int deg;

/////////////////////////////////////////////////////////////////////////// 
// === Estructura extendida para datos de forecast (v14 — ajustes pop%, viento km/h) ===
/////////////////////////////////////////////////////////////////////////// 
struct ForecastData {
  char hora[6];           // HH:MM local
  char icono[4];          // Código de icono (ej. 04d)
  float temperatura;      // Temperatura actual
  float temp_min;         // Temperatura mínima
  float temp_max;         // Temperatura máxima
  float feels_like;       // Sensación térmica
  float pressure;         // Presión atmosférica (hPa)
  int humidity;           // Humedad relativa (%)
  int pop;                // Probabilidad de lluvia (%) — convertido de 0–1 a 0–100
  float rain_3h;          // Precipitación (mm/3h)
  float wind_speed;       // Velocidad viento (km/h) — convertida desde m/s
  int wind_deg;           // Dirección viento (grados)
  int clouds;             // Nubosidad (%)
  char description[40];   // Descripción textual
};

#define FORECAST_COUNT 3
ForecastData forecast[FORECAST_COUNT];
unsigned long lastForecastTime = 0;  // millis() última actualización

// Globales para sunrise/sunset/timezone (usadas por forecast)
unsigned long gSunrise = 0;
unsigned long gSunset  = 0;
long gTimezoneShift    = 0;

// === Variables globales para el sol ===
String inicioSol = "--:--";
String finalSol  = "--:--";
String tiempoSol = "--:--";

// ===============================================================
String weatherLoc;
String weatherIcon = "01d";                     // por ejemplo, "01d", "02n"
String weatherDesc = "Cielo despejado";         // texto descriptivo
const String units = "metric";                  // "metric" = °C
const String lang  = "es";                      // Idioma de descripción

/////////////////////////////////////////////////////////////////
// ===================== TIMING =====================
const unsigned long weatherUpdateInterval = 600000; // 10 minutos
const unsigned long screenInterval = 8000;          // cambio de pantalla cada 8"
const unsigned long previsionInterval = 4000;       // cambio cada 4" cuando se muestra la previsión
long timeConnect;

// ===================== DISPLAY OLED =====================

#include <Adafruit_GFX.h>
#include "Adafruit_SSD1306.h" // Copy the supplied version of Adafruit_SSD1306.cpp and Adafruit_ssd1306.h to the sketch folder
#define OLED_RESET -1
Adafruit_SSD1306 display(OLED_RESET);

// ===================== VARIABLES =====================
unsigned long lastWeatherUpdate = 0;
unsigned long lastScreenChange = 0;
unsigned long lastIntervalChange = 0;
int currentScreen = 0;
bool nScreen = false;
int s;

// Turn on debug statements to the serial output
#define DEBUG  1
#if  DEBUG
#define PRINT(s, x) { Serial.print(F(s)); Serial.print(x); }
#define PRINTS(x) Serial.print(F(x))
#define PRINTX(x) Serial.println(x, HEX)
#else
#define PRINT(s, x)
#define PRINTS(x)
#define PRINTX(x)
#endif

// =======================================================
// CONFIGURACIÓN ADAPTATIVA DE PANTALLA OLED
// =======================================================
bool pantallaGrande = false;   // false = 64x48, true = 128x64
// Offset Wind
int vX = 0;
int vY = 0;
int kX = 0;
int kY = 0;

// Offset Weather 
int gX = 0;
int gY = 0;
int gR = 0;

void configurarPantalla() {
  int w = display.width();
  int h = display.height();

  if (w >= 128 && h >= 64) {
    PRINTS("\n*** OLED 128x64 ***");
    pantallaGrande = true;
    // Offset Wind
    vX = 5;
    vY = 16;
    kX = 22;
    kY = 8;
    
    // Offset Weather 
    gX = 42;
    gY = 5;
    gR = 2;
  } else {
    PRINTS("\n*** OLED 64x48 ****");
  }
}
//////////////////////////////////////////////////////////////////
// === Mostrar información de memoria libre ===
//////////////////////////////////////////////////////////////////
void imprimirHeapInfo() {
#if defined(ESP8266)
  Serial.printf("[ESP8266] Memoria libre inicial: %u bytes\n", ESP.getFreeHeap());
#elif defined(ESP32)
  Serial.printf("[ESP32] Memoria libre inicial: %u bytes\n", ESP.getFreeHeap());
#endif
}
/////////////////////////////////////////////////////////////////////////////////////////////
// ===================== SETUP =====================
/////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  configurarPantalla();
  PRINT("\nESP_NTP_Weather ",HWversion);
  PRINTS("\nConectando WiFi.");

  timeConnect = millis();

  //////////////////
  // Sólo ESP-01S //
  //////////////////
  //Wire.begin(0, 2);                           // SDA = GPIO0, SCL = GPIO2
  /////////////////////////////////////////////////////////////////////////////////

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C
  
  /////////////////////////////////////////////////////////////////////////////////
  // 🔄 Gira la pantalla 180 grados (Kit AliExpress)
  //display.setRotation(2);
  /////////////////////////////////////////////////////////////////////////////////
  
  display.setTextColor(WHITE);
  display.clearDisplay();

  if (pantallaGrande) {
    display.setTextSize(2);
    display.setCursor(0,0);   
    display.println(F("NTP WEATHE"));

    display.setTextSize(3);
    display.setCursor(0,32);   
    display.println(HWversion);

  }else {
    display.setTextSize(2);
    display.setCursor(15,0);   
    display.println(F("NTP"));
  
    display.setTextSize(1);   
    display.setCursor(10,16);   
    display.println(F("WEATHER"));
    display.setCursor(11,26);   
    display.println(HWversion);
  
    display.setCursor(10,38);   
    display.println(F("Sync..."));
  }
  display.display();
  display_flash();
//------------------------------
  //WiFiManager intialisation. Once completed there is no need to repeat the process on the current board
  WiFiManager wifiManager;
  display_AP_wifi();

  // A new OOB ESP32 will have no credentials, so will connect and not need this to be uncommented and compiled in, a used one will, try it to see how it works
  // Uncomment the next line for a new device or one that has not connected to your Wi-Fi before or you want to reset the Wi-Fi connection
  // wifiManager.resetSettings();
  // Then restart the ESP32 and connect your PC to the wireless access point called 'ESP32_AP' or whatever you call it below
  // Next connect to http://192.168.4.1/ and follow instructions to make the WiFi connection

  // Set a timeout until configuration is turned off, useful to retry or go to sleep in n-seconds
  wifiManager.setTimeout(180);
  
  //fetches ssid and password and tries to connect, if connections succeeds it starts an access point with the name called "ESP32_AP" and waits in a blocking loop for configuration
  if (!wifiManager.autoConnect("ESP32_AP")) {
    PRINTS("\nFailed to connect and timeout occurred");
    display_AP_wifi();
    display_flash();
    ESP.restart();
  }
  // At this stage the WiFi manager will have successfully connected to a network,
  // or if not will try again in 180-seconds
  //---------------------------------------------------------
  // 
  PRINT("\n>>> Connection Delay(ms): ",millis()-timeConnect);
  if(millis()-timeConnect > 30000) {
    PRINTS("\nTimeOut connection, restarting!!!");
    ESP.restart();
  }

  // Print the IP of Webpage
  PRINT("\nUse this URL to connect -> http://",WiFi.localIP());
  PRINTS("/");
  
  display_ip();
  display_flash();


  // Syncronize Time and Date
  if (!SetupTime()) {
    PRINTS("\nTimeOut SetupTime, restarting!!!");
    ESP.restart();
  }

  
  // Muestra memoria libre inicial y antes de parsear el JSON
  imprimirHeapInfo();
  
  // Carga la configuración de la EEPROM (API key, city, zone, etc.)
  EEPROM.begin(EEPROM_SIZE);
  loadConfig();
  
  obtenerClima();
  lastWeatherUpdate = millis();
  lastScreenChange = millis();
  lastIntervalChange = millis();
  
  // Forzar actualización inicial del forecast (en frío)
  PRINTS("\nForzando actualización de forecast inicial...");
  // Hacemos que parezca que la última actualización fue hace más de 2h
  //lastForecastTime = millis() - (2UL * 60UL * 60UL * 1000UL) - 1000UL;
  lastForecastTime = 0;  // forzar actualización inmediata
  actualizarForecast();  // Ejecuta la consulta inicial
   
  // Habilita las entradas Web
  checkServer();
}
///////////////////////////////////////////////////////////////////////
// ===================== LOOP =====================
///////////////////////////////////////////////////////////////////////
void loop() {
  // Wait for a client to connect and when they do process their requests
  server.handleClient();
  UpdateLocalTime();

  //Comprueba si se debe resincronizar el reloj del ESP32 
  if (CurrentTime == "03:10:10") {
   // Syncronize Time and Date
   ESP.restart();
  }

  // En modo Test, sólo muestra la lista de los iconos
  if (Test) {
      display.clearDisplay();
      display.setTextColor(WHITE);
      if (pantallaGrande) {
        display.setTextSize(2);
      }else {
        display.setTextSize(1);
      }
      display.setCursor(0, 0);
      if (s < 5) {
        display.println("Sol_D");
        dibujarSol();
     }else if (s < 11) {
        display.println("Sol_N");
        dibujarNoche();
      }else if (s < 16) {
        display.println("Nube+Sol_D");
        dibujarNubeSol();
      }else if (s < 21) {
        display.println("Nube+Sol_N");
        dibujarNubeNoche();
      }else if (s < 27){
        display.println("NubeLluvia");
        dibujarLluvia();
      }else if (s < 32) {
        display.println("NubeLluv_D");
        dibujarLluviaDia();      
      }else if (s < 38) {
        display.println("NubeLluv_N");
        dibujarLluviaNoche();      
      }else if (s < 43) {
        display.println("Nube+Rayo");
        dibujarRayo();
      }else if (s < 49) {
        display.println("Nube+Nieve");
        dibujarNieve();
      }else if (s < 54) {
        display.println("Bruma");
        dibujarBruma();
      }else {
        display.println("Nube");
        dibujarNube();
      }
      // Sensación Térmica
      cagarFeels();
      // Velocidad del viento
      cargarViento();
  }else {
    // Si se está mostrando la fecha y hora, se actualiza
    if (currentScreen == 2) {
      Oled_Time();
    // Si se está mostrando la previsión del tiempo, gestiona los cambios
    }else if (!Test && (millis() - lastIntervalChange > previsionInterval)) {
      // Alterna la información de la previsión a mostrar
      if (currentScreen == 3) {
        previsionWeather();
      }else if (currentScreen == 4) {
        previsionWeather2();
      }
    }
  }


  // Vuelve a cargar el estado actual del clima 
  if (millis() - lastWeatherUpdate > weatherUpdateInterval) {
    obtenerClima();
    // Actualiza cada 2h solo si corresponde
    actualizarForecast();  
  }


  // Selecciona la pantalla a mostrar
  if (millis() - lastScreenChange > screenInterval) {
    currentScreen = (currentScreen + 1) % 6; // ahora 6 pantallas
    if (!Test) mostrarPantalla(currentScreen);
    lastScreenChange = millis();
  }
}
// ==================================================================
    /*
    // City
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city +
                 "&appid=" + apiKey + "&units=" + units + "&lang=es";
    // Coordenadas
    String url = "http://api.openweathermap.org/data/2.5/weather?lat=" + lat + "&lon=" + lon + "&appid=" + apiKey
             + "&units=" + units + "&lang=" + lang;
    */
//////////////////////////////////////////////////////////////////
// ===================== FUNCIÓN: OBTENER CLIMA =====================
//////////////////////////////////////////////////////////////////
void obtenerClima() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    // City ID
    String url = "http://api.openweathermap.org/data/2.5/weather?id=" + cityID +
                 "&appid=" + apiKey + "&units=" + units + "&lang=" + lang;
 
    #ifdef ESP32
      PRINT("\nSolicitando ESP32: ",url);
      http.begin(url);  // ESP32 puede usar directamente la URL
    #else
      PRINT("\nSolicitando ESP8286: ",url);
      WiFiClient client;
      http.begin(client, url);  // ESP8266 necesita cliente explícito
    #endif

    http.setTimeout(15000);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      PRINT("\n>>> payload: ", payload);

      // Parseo seguro (tamaño ampliado para evitar NoMemory en ESP32/ESP8266)
      DynamicJsonDocument doc(4096);
      DeserializationError err = deserializeJson(doc, payload);
      if (err) {
        PRINT("\nError parseando JSON weather: ", err.c_str());
        mostrarError("JSON");
        limpiarDatosClima();
        http.end();
        return;
      }

      // Valores principales
      temp = doc["main"]["temp"].as<float>();
      feels = doc["main"]["feels_like"].as<float>();
      humidity = doc["main"]["humidity"].as<float>();
      pressure = doc["main"]["pressure"].as<float>();
      wind = (doc["wind"]["speed"].as<float>()) * 3.6; // ms >>> km/h
      deg = doc["wind"]["deg"].as<int>();
      weatherDesc = doc["weather"][0]["description"].as<String>();
      weatherDesc.toUpperCase();  // convierte a mayúsculas
      weatherLoc = doc["name"].as<String>();
      if (weatherLoc.length() > 10) weatherLoc = weatherLoc.substring(0, 10);
      weatherLoc.toUpperCase();
      weatherIcon = doc["weather"][0]["icon"].as<String>();

      // —— EXTRAER sunrise/sunset/timezone y guardarlos globalmente ——
      if (!doc["sys"].isNull()) {
        if (doc["sys"]["sunrise"].is<unsigned long>()) {
          gSunrise = (unsigned long)doc["sys"]["sunrise"].as<unsigned long>();
        } else {
          gSunrise = 0;
        }
        if (doc["sys"]["sunset"].is<unsigned long>()) {
          gSunset = (unsigned long)doc["sys"]["sunset"].as<unsigned long>();
        } else {
          gSunset = 0;
        }
      }
      if (doc["timezone"].is<long>()) {
        gTimezoneShift = (long)doc["timezone"].as<long>();
      } else {
        gTimezoneShift = 0;
      }

      Serial.printf("\n[INFO] Guardados globals sunrise=%lu sunset=%lu tz=%ld\n",
                    gSunrise, gSunset, gTimezoneShift);

      // Actualiza Strings legibles de amanecer/puesta/duración
      actualizarSunTimes(gSunrise, gSunset, gTimezoneShift);
      Serial.printf("[INFO] inicioSol='%s' finalSol='%s' tiempoSol='%s'\n",
                    inicioSol.c_str(), finalSol.c_str(), tiempoSol.c_str());

      // Actualizar pantalla con los nuevos datos
      mostrarPantalla(currentScreen);

    } else {
      PRINT("\nError HTTP: ", httpCode);
      mostrarError(String(httpCode));
      limpiarDatosClima();
      currentScreen = 0;
      lastScreenChange = millis();
      lastIntervalChange = millis();
    }
    http.end();
    lastWeatherUpdate = millis();
  } else {
    mostrarError("WiFi");
  }
}
//////////////////////////////////////////////////////////////////
// === LIMPIAR DATOS DE CLIMA (usado cuando falla la consulta) ===
//////////////////////////////////////////////////////////////////
void limpiarDatosClima() {
  temp = 0.0;
  feels = 0.0;
  humidity = 0.0;
  pressure = 0.0;
  wind = 0.0;
  deg = 0;
  weatherDesc = "---";
  weatherLoc = "---";
  weatherIcon = "--";
}
//////////////////////////////////////////////////////////////////////////////////////
// === ACTUALIZAR FORECAST (v14 — ajustes pop %, viento km/h y salida completa) === //
//////////////////////////////////////////////////////////////////////////////////////
void actualizarForecast() {
  unsigned long ahora = millis();
  const unsigned long intervaloForecast = 7200000UL; // 2h

  if (lastForecastTime != 0 && (ahora - lastForecastTime) < intervaloForecast) {
    PRINTS("\nForecast reciente, usando datos guardados.");
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    PRINTS("\nSin WiFi: no se puede actualizar forecast.");
    return;
  }

  String url = "http://api.openweathermap.org/data/2.5/forecast?id=" + cityID +
               "&appid=" + apiKey + "&units=metric&lang=" + lang;

  PRINTS("\nSolicitando forecast: ");
  Serial.print(url);

  WiFiClient client;
  HTTPClient http;
#ifdef ESP32
  http.begin(url);
#else
  http.begin(client, url);
#endif
  http.setTimeout(25000);
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    PRINT("\nError HTTP forecast: ", httpCode);
    http.end();
    return;
  }

  WiFiClient *stream = http.getStreamPtr();
  String buffer;
  int count = 0;
  int searchPos = 0;

  unsigned long sunrise = gSunrise;
  unsigned long sunset  = gSunset;
  long timezoneShift    = gTimezoneShift;

  PRINTS("\nLeyendo forecast por streaming (v14 extendida)...\n");
  Serial.printf("[INFO] Sunrise global=%lu sunset=%lu tz=%ld\n", sunrise, sunset, timezoneShift);

  // === Bucle de lectura ===
  while (stream->connected() && count < FORECAST_COUNT) {
    while (stream->available()) {
      char c = stream->read();
      if (c != '\r') buffer += c;
    }
    if (buffer.length() < 300) { delay(1); continue; }

    bool foundAny = false;
    while (true) {
      int idxHora = buffer.indexOf("\"dt_txt\":\"", searchPos);
      if (idxHora < 0) break;

      // --- Buscar timestamp (dt) ---
      int idxDt = buffer.lastIndexOf("\"dt\":", idxHora);
      unsigned long dtForecast = 0;
      if (idxDt > 0) {
        int endDt = buffer.indexOf(",", idxDt);
        if (endDt > 0) dtForecast = buffer.substring(idxDt + 5, endDt).toInt();
      }

      // dt_txt ya contiene hora local
      int startH = idxHora + 10;
      int endH = buffer.indexOf("\"", startH);
      if (endH < 0) break;
      String horaFull = buffer.substring(startH, endH);
      String horaGuardar = (horaFull.length() >= 16) ? horaFull.substring(11, 16) : "--:--";

      int nextHora = buffer.indexOf("\"dt_txt\":\"", endH + 10);
      int searchEnd = (nextHora > 0) ? nextHora : buffer.length();

      // === Localizar principales campos ===
      int idxIcon  = buffer.indexOf("\"icon\":\"", idxHora);
      int idxTemp  = buffer.indexOf("\"temp\":", idxHora);
      int idxTmin  = buffer.indexOf("\"temp_min\":", idxHora);
      int idxTmax  = buffer.indexOf("\"temp_max\":", idxHora);
      int idxFeels = buffer.indexOf("\"feels_like\":", idxHora);
      int idxPress = buffer.indexOf("\"pressure\":", idxHora);
      int idxHum   = buffer.indexOf("\"humidity\":", idxHora);
      int idxPop   = buffer.indexOf("\"pop\":", idxHora);
      int idxRain  = buffer.indexOf("\"rain\":", idxHora);
      int idxWindS = buffer.indexOf("\"speed\":", idxHora);
      int idxWindD = buffer.indexOf("\"deg\":", idxHora);
      int idxCloud = buffer.indexOf("\"clouds\":", idxHora);
      int idxDesc  = buffer.indexOf("\"description\":\"", idxHora);

#if DEBUG
      Serial.printf("\n[DEBUG #%d] dtForecast=%lu  HoraTxt(local)=%s\n", count, dtForecast, horaGuardar.c_str());
      int dbgStart = max(0, idxHora - 80);
      size_t dbgLen = min((size_t)400, (size_t)(buffer.length() - dbgStart));
      Serial.println(buffer.substring(dbgStart, dbgStart + dbgLen));
      Serial.println(F("-----------------------------"));
#endif

      // === Extracción de valores ===
      String icono = (idxIcon > 0) ? buffer.substring(idxIcon + 8, idxIcon + 11) : "--";
      float temp   = (idxTemp > 0)  ? buffer.substring(idxTemp + 7, buffer.indexOf(",", idxTemp)).toFloat() : NAN;
      float tmin   = (idxTmin > 0)  ? buffer.substring(idxTmin + 11, buffer.indexOf(",", idxTmin)).toFloat() : NAN;
      float tmax   = (idxTmax > 0)  ? buffer.substring(idxTmax + 11, buffer.indexOf(",", idxTmax)).toFloat() : NAN;
      float feels  = (idxFeels > 0) ? buffer.substring(idxFeels + 13, buffer.indexOf(",", idxFeels)).toFloat() : NAN;
      float press  = (idxPress > 0) ? buffer.substring(idxPress + 11, buffer.indexOf(",", idxPress)).toFloat() : NAN;
      int hum      = (idxHum > 0)   ? buffer.substring(idxHum + 11, buffer.indexOf(",", idxHum)).toInt() : -1;

      // --- POP (convertido a %) ---
      int pop = 0;
      if (idxPop > 0) {
        float popF = buffer.substring(idxPop + 6, buffer.indexOf(",", idxPop)).toFloat();
        pop = int(popF * 100.0);
      }

      // --- Lluvia (mm/3h)
      float rain3h = 0;
      if (idxRain > 0) {
        int idx3h = buffer.indexOf("\"3h\":", idxRain);
        if (idx3h > 0) rain3h = buffer.substring(idx3h + 5, buffer.indexOf("}", idx3h)).toFloat();
      }

      // --- Viento (m/s → km/h)
      float windS = NAN;
      if (idxWindS > 0) {
        float windMs = buffer.substring(idxWindS + 8, buffer.indexOf(",", idxWindS)).toFloat();
        windS = windMs * 3.6;
      }

      int windD = (idxWindD > 0) ? buffer.substring(idxWindD + 6, buffer.indexOf("}", idxWindD)).toInt() : -1;
      int clouds = 0;
      int idxAll = buffer.indexOf("\"all\":", idxCloud);
      if (idxAll > 0) clouds = buffer.substring(idxAll + 6, buffer.indexOf("}", idxAll)).toInt();
      String desc = (idxDesc > 0) ? buffer.substring(idxDesc + 15, buffer.indexOf("\"", idxDesc + 15)) : "--";

      // === Guardar los valores extraídos ===
      if (horaGuardar != "--:--" && icono != "--" && !isnan(temp)) {
        horaGuardar.toCharArray(forecast[count].hora, sizeof(forecast[count].hora));
        icono.toCharArray(forecast[count].icono, sizeof(forecast[count].icono));
        desc.toCharArray(forecast[count].description, sizeof(forecast[count].description));

        forecast[count].temperatura = temp;
        forecast[count].temp_min    = tmin;
        forecast[count].temp_max    = tmax;
        forecast[count].feels_like  = feels;
        forecast[count].pressure    = press;
        forecast[count].humidity    = hum;
        forecast[count].pop         = pop;
        forecast[count].rain_3h     = rain3h;
        forecast[count].wind_speed  = windS;
        forecast[count].wind_deg    = windD;
        forecast[count].clouds      = clouds;

        // --- Nueva salida completa ---
        Serial.printf("Forecast %d  Hora:%s  T=%.1f°C  Feels=%.1f  Min=%.1f  Max=%.1f  Hum=%d%%  Pop=%d%%  Rain=%.1fmm  Viento=%.1fkm/h (%d°)  Nubes=%d%%  Icono=%s  Desc=%s\n",
                      count, forecast[count].hora, temp, feels, tmin, tmax, hum, pop, rain3h, windS, windD, clouds,
                      forecast[count].icono, forecast[count].description);

        count++;
        foundAny = true;
        searchPos = (buffer.indexOf(",", idxHora) > 0) ? buffer.indexOf(",", idxHora) : idxHora + 1;
        if (searchPos > 2000) {
          buffer = buffer.substring(searchPos);
          searchPos = 0;
        }
        if (count >= FORECAST_COUNT) break;
      } else break;
    }

    if (!foundAny && buffer.length() > 8000) {
      int cutPos = buffer.length() - 4000;
      buffer = buffer.substring(cutPos);
      searchPos = max(0, searchPos - cutPos);
    }
    delay(1);
  }

  http.end();

  if (count > 0) {
    lastForecastTime = ahora;
    PRINT("\nForecast actualizado correctamente (", String(count));
    PRINTS(" tramos).");
  } else {
    PRINTS("\nNo se pudieron extraer datos válidos del forecast.");
  }
}
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// ===============================================================
// Función: actualizarSunTimes
// - Calcula inicioSol, finalSol y tiempoSol (formato "HH:MM", 5 chars)
// - Usa sunrise/sunset en epoch UTC (unsigned long) y timezoneShift en segundos
// - Si sunrise/sunset == 0 o timezoneShift desconocido -> deja "--:--"
// ===============================================================
////////////////////////////////////////////////////////////////////////////
void actualizarSunTimes(unsigned long sunriseUTC, unsigned long sunsetUTC, long timezoneShiftSec) {
  // Valores por defecto si no hay datos
  inicioSol = "--:--";
  finalSol  = "--:--";
  tiempoSol = "--:--";

  // Validación básica
  if (sunriseUTC == 0 || sunsetUTC == 0) {
    return;
  }

  // Convertir a tiempo local sumando timezone shift (segundos)
  unsigned long sunriseLocalEpoch = sunriseUTC + (unsigned long)timezoneShiftSec;
  unsigned long sunsetLocalEpoch  = sunsetUTC  + (unsigned long)timezoneShiftSec;

  // Convertir a tm_struct usando gmtime() porque ya aplicamos el shift
  time_t t1 = (time_t)sunriseLocalEpoch;
  time_t t2 = (time_t)sunsetLocalEpoch;

  struct tm tm1;
  struct tm tm2;

  // gmtime_r / gmtime may or may not be available; usar gmtime seguro
  // (en ESPs gmtime_r no siempre existe): usar gmtime() y copiar resultado.
  struct tm *ptm;
  ptm = gmtime(&t1);
  if (ptm == NULL) return;
  tm1 = *ptm;

  ptm = gmtime(&t2);
  if (ptm == NULL) return;
  tm2 = *ptm;

  // Formatear "HH:MM" con espacio en la decena si hora < 10
  char bufStart[6]; // " H:MM" o "HH:MM" + '\0'
  char bufEnd[6];
  snprintf(bufStart, sizeof(bufStart), "%2d:%02d", tm1.tm_hour, tm1.tm_min);
  snprintf(bufEnd,   sizeof(bufEnd),   "%2d:%02d", tm2.tm_hour, tm2.tm_min);

  inicioSol = String(bufStart);
  finalSol  = String(bufEnd);

  // Duración del día = sunsetLocalEpoch - sunriseLocalEpoch
  if (sunsetLocalEpoch > sunriseLocalEpoch) {
    unsigned long diff = sunsetLocalEpoch - sunriseLocalEpoch;
    int dh = diff / 3600;
    int dm = (diff % 3600) / 60;
    char bufDur[6];
    // Aseguramos formato HH:MM (si dh >= 100 truncar a 99)
    if (dh > 99) dh = 99;
    snprintf(bufDur, sizeof(bufDur), "%2d:%02d", dh, dm);
    tiempoSol = String(bufDur);
  } else {
    tiempoSol = "--:--";
  }
}
//////////////////////////////////////////////////////////////////
// ===================== FUNCIÓN: MOSTRAR PANTALLAS =====================
//////////////////////////////////////////////////////////////////
void mostrarPantalla(int pantalla) {
  display.clearDisplay();
  display.setTextColor(WHITE);

  switch (pantalla) {
    // --- Pantalla 1: Localidad, Temperatura, Humedad y Presión ---
    case 0:
      // Localidad
      if (pantallaGrande) {
        display.setTextSize(2);
      }else {
        display.setTextSize(1);
      }
      display.setCursor(0, 0);
      display.println(weatherLoc);
      if (!pantallaGrande) {
        display.drawLine(0, 8, 63, 8, WHITE);
      }
      
      // Temperatura
      if (pantallaGrande) {
        display.setTextSize(3);
        display.setCursor(8, 20);
      }else {
        display.setTextSize(2);
        display.setCursor(0, 12);
      }
      display.print(temp, 1);
      display.print((char)247);
      if (pantallaGrande) display.println(F("C"));
 
      // Humedad 
      display.setTextSize(1);
      if (pantallaGrande) {
        display.setCursor(0, 48);
      }else {
        display.setCursor(10, 30);
      }
      display.print("Hr: ");
      display.print(humidity, 0);

      // Presión Atmosférica
      if (pantallaGrande) {
        display.print("% - ");
      }else {
        display.println("%");
        display.setCursor(4, 40);
      }
      display.print(pressure, 0);
      display.println(" mbar");
      display.display();
      break;

    // --- Pantalla 2: Descripción, Sensación térmica, Icono y Viento ---
    case 1:
      if (pantallaGrande) {
        display.setTextSize(2);
      }else {
        display.setTextSize(1);
      }
      display.setCursor(0, 0);
      display.println(weatherDesc.substring(0, 10));

      // Sensación Térmica
      cagarFeels();

      // Dibuja icono simple según el código de OpenWeatherMap
      //https://openweathermap.org/img/wn/01d@2x.png
      if (weatherIcon.indexOf("01d") >= 0) dibujarSol();
      else if (weatherIcon.indexOf("01n") >= 0) dibujarNoche();
      else if (weatherIcon.indexOf("02d") >= 0) dibujarNubeSol();
      else if (weatherIcon.indexOf("02n") >= 0) dibujarNubeNoche();
      else if (weatherIcon.indexOf("09") >= 0) dibujarLluvia();
      else if (weatherIcon.indexOf("10d") >= 0) dibujarLluviaDia();
      else if (weatherIcon.indexOf("10n") >= 0) dibujarLluviaNoche();
      else if (weatherIcon.indexOf("11") >= 0) dibujarRayo();
      else if (weatherIcon.indexOf("13") >= 0) dibujarNieve();
      else if (weatherIcon.indexOf("50") >= 0) dibujarBruma();
      else dibujarNube();

      // Velocidad del viento
      cargarViento();
      break;

    // --- Pantalla 3: Fecha y Hora ---
    case 2:
      Oled_Time();
      break;

    // --- Pantalla 4: Forecast >>> Cielo y Nubosidad % 
    case 3:   
      nScreen = false;
      previsionWeather();
      break;

    // --- Pantalla 5: Forecast >>> Precipitación % y Temperatura
    case 4:   
      nScreen = false;
      previsionWeather2();
      break;

    // --- Pantalla 6: Salida y puesta de Sol + duración del día
    case 5:   
      horasSol();
      break;
  }
}
//////////////////////////////////////////////////////////////////
// ===================== FUNCIONES DE ICONOS =====================
//////////////////////////////////////////////////////////////////
void drawWindArrow(float deg) {
  float cx = 20+(vX*2); // centro de rotación (x)
  float cy = 42+vY;     // centro de rotación (y)
  float len = 10.0+gR;  // longitud total de la flecha
  float head = 5.0+gR;  // tamaño de la cabeza (pico)

  // Convertir el ángulo a radianes (flecha apunta hacia donde VA el viento)
  float angle = radians(deg + 180);

  // Calcular extremos delantero y trasero de la flecha
  float xFront = cx + (len / 2) * sin(angle);
  float yFront = cy - (len / 2) * cos(angle);
  float xBack = cx - (len / 2) * sin(angle);
  float yBack = cy + (len / 2) * cos(angle);

  // Calcular alas (laterales del pico)
  float angleLeft = angle + radians(150);
  float angleRight = angle - radians(150);

  float xLeft = xFront + head * sin(angleLeft);
  float yLeft = yFront - head * cos(angleLeft);
  float xRight = xFront + head * sin(angleRight);
  float yRight = yFront - head * cos(angleRight);

  // Dibujar cuerpo central
  display.drawLine(xBack, yBack, xFront, yFront, WHITE);

  // Dibujar triángulo relleno (cabeza)
  display.fillTriangle(
    xFront, yFront,
    xLeft, yLeft,
    xRight, yRight,
    WHITE
  );
}
//////////////////////////////////////////////////////////////////////
void cagarFeels() {
  // Sensación térmica
  if (pantallaGrande) {
    display.setCursor(0, 16);
    display.println("Feels");
    display.setCursor(0, 32);
  }else {
    display.drawLine(0, 8, 63, 8, WHITE);
    display.setCursor(0, 12);
    display.println("Feels");
    display.setCursor(0, 22);
  }
  display.print(feels,1);
  display.println((char)247);
}
//////////////////////////////////////////////////////////////////////
void cargarViento() {
  // Gráfico: velocidad del viento
  display.drawCircle(3+vX, 37+vY, 2, WHITE);
  display.drawLine(0, 39+vY, 1+vX, 39+vY, WHITE);
  display.drawLine(0, 38+vY, 3+vX, 38+vY, BLACK);
  
  display.drawCircle(9+vX, 39+vY, 2, WHITE);
  display.drawLine(0, 41+vY, 7+vX, 41+vY, WHITE);
  display.drawLine(0, 40+vY, 9+vX, 40+vY, BLACK);
  
  display.drawCircle(5+vX, 45+vY, 2, WHITE);
  display.drawLine(0, 43+vY, 3+vX, 43+vY, WHITE);
  display.drawLine(0, 44+vY, 5+vX, 44+vY, BLACK);

  // Flecha dirección viento
  if (Test) deg++; 
  if (deg > 359) deg = 0;
  drawWindArrow(deg);
 
  // Velocidad en kmh
  display.setCursor(28+kX, 40+kY);
  display.print(wind, 0);
  if (wind < 99.5) display.print(" ");
  display.println("kmh");
  display.display();
}
//////////////////////////////////////////////////////////////////////
void dibujarSol() {
  display.fillCircle(49+gX, 23+gY, 7+gR,WHITE);
  // Destellos SOL día
  for (int i = 0; i < 360; i += 45) {
    float rad = i * M_PI / 180.0;  // conversión manual a radianes
    float x = 49+gX + cos(rad) * (9+gR);
    float y = 23+gY + sin(rad) * (9+gR);
    display.drawPixel((int)x, (int)y, WHITE);
    x = 49+gX + cos(rad) * (10+gR);
    y = 23+gY + sin(rad) * (10+gR);
    display.drawPixel((int)x, (int)y, WHITE);
  }
}
//////////////////////////////////////////////////////////////////////
void dibujarNoche() {
  display.drawCircle(49+gX, 23+gY, 7+gR,WHITE);
}
//////////////////////////////////////////////////////////////////////
void dibujarNube() {
  display.fillCircle(58+gX, 24+gY, 6+gR, WHITE);
  display.fillCircle(54+gX, 22+gY, 6+gR, WHITE);
  display.fillCircle(46+gX, 22+gY, 8+gR, WHITE);
}
//////////////////////////////////////////////////////////////////////
void dibujarNubeSol() {
  dibujarNubeDia();
  // Destellos SOL día
  for (int i = 0; i < 360; i += 45) {
    float rad = i * M_PI / 180.0;  // conversión manual a radianes
    float x = 38+gX + cos(rad) * (6+gR);
    float y = 20+gY + sin(rad) * (6+gR);
    display.drawPixel((int)x, (int)y, WHITE);
  }
}
//////////////////////////////////////////////////////////////////////
void dibujarNubeDia() {
  dibujarNube();
  display.fillCircle(38+gX, 20+gY, 5+gR, BLACK);
  display.fillCircle(38+gX, 20+gY, 4+gR, WHITE);
}
//////////////////////////////////////////////////////////////////////
void dibujarNubeNoche() {
  dibujarNube();
  display.fillCircle(38+gX, 20+gY, 5+gR, BLACK);
  display.drawCircle(38+gX, 20+gY, 4+gR, WHITE);
}
//////////////////////////////////////////////////////////////////////
void dibujarLluviaDia() {
  dibujarNubeSol();
  ponerLluvia();
}
//////////////////////////////////////////////////////////////////////
void dibujarLluviaNoche() {
  dibujarNubeNoche();
  ponerLluvia();
}
//////////////////////////////////////////////////////////////////////
void dibujarLluvia() {
  dibujarNube();
  ponerLluvia();
}
//////////////////////////////////////////////////////////////////////
void ponerLluvia() {
  display.drawLine(45+gX, 30+gY+gR, 43+gX, 36+gY+gR, WHITE);
  display.drawLine(49+gX, 30+gY+gR, 47+gX, 36+gY+gR, WHITE);
  display.drawLine(53+gX, 30+gY+gR, 51+gX, 36+gY+gR, WHITE);
}
//////////////////////////////////////////////////////////////////////
void dibujarRayo() {
  dibujarNube();
  display.drawLine(54+gX, 22+gY+gR, 50+gX, 32+gY+gR, WHITE);
  display.drawLine(50+gX, 32+gY+gR, 56+gX, 32+gY+gR, WHITE);
  display.drawLine(56+gX, 32+gY+gR, 54+gX, 37+gY+gR, WHITE);
}
//////////////////////////////////////////////////////////////////////
void dibujarNieve() {
  dibujarNube();
  display.drawPixel(48+gX, 32+gY+gR, WHITE);
  display.drawPixel(50+gX, 34+gY+gR, WHITE);
  display.drawPixel(46+gX, 34+gY+gR, WHITE);
  display.drawPixel(48+gX, 36+gY+gR, WHITE);
}
//////////////////////////////////////////////////////////////////////
void dibujarBruma()  {
  display.drawLine(46+gX, 14+gY, 52+gX, 14+gY, WHITE);
  display.drawLine(44+gX, 17+gY, 53+gX, 17+gY, WHITE);
  display.drawLine(46+gX, 20+gY, 58+gX, 20+gY, WHITE);
  display.drawLine(42+gX, 23+gY, 54+gX, 23+gY, WHITE);
  display.drawLine(45+gX, 26+gY, 55+gX, 26+gY, WHITE);
  display.drawLine(48+gX, 29+gY, 52+gX, 29+gY, WHITE);
}
// ===================== ERROR =====================
void mostrarError(String msg) {
  display.clearDisplay();
  display.setTextSize(2);
  if (pantallaGrande) {
    display.setCursor(32, 0);
  }else {
    display.setCursor(0, 0);
  }
  display.println(F("ERROR"));

  if (pantallaGrande) {
    display.setTextSize(5);
    display.setCursor(16, 20);
  }else {
    display.setCursor(4, 20);
    display.setTextSize(3);
  }
  display.println(msg);
  display.display();
}
///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
boolean UpdateLocalTime() {
  struct tm timeinfo;
  time_t now;
  time(&now);

  //See http://www.cplusplus.com/reference/ctime/strftime/
  // %w >>> Weekday as a decimal number with Sunday as 0 (0-6)
  static String txWDay[7] = {"DOM","LUN","MAR","MIE","JUE","VIE","SAB"};
  char output[20];
  
  //////////////////////////
  //  CARGA Fecha y Hora  //
  //////////////////////////
  strftime(output, 20, "%w", localtime(&now));
  String dTX = txWDay[atoi(output)];
  strftime(output, 20,"%d/%m", localtime(&now));
  CurrentDate = dTX + String(". ") + output;

  strftime(output, 20, "%H:%M:%S", localtime(&now));
  CurrentTime = output;
  
  // Para utilizar en TEST de Iconos
  if (Test) s = (CurrentTime.substring(6,8)).toInt();

  return true;
}
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//------------------ OLED DISPLAY -----------------//
void Oled_Time() { 
  display.clearDisplay();
  if (pantallaGrande) {
    display.setCursor(1,0);   // center date display
    display.setTextSize(2);   
    display.println(CurrentDate);
  
    display.setTextSize(3);   
    if (CurrentTime.startsWith("0")){
      display.setCursor(28,16);  // center Time display
      display.println(CurrentTime.substring(1,5));
    }else {
      display.setCursor(18,16);
      display.println(CurrentTime.substring(0,5));
    }
    display.setCursor(28,40); // center Time display
  }else {
    display.setCursor(2,0);   // center date display
    display.setTextSize(1);   
    display.println(CurrentDate);
    display.drawLine(0, 8, 63, 8, WHITE);
  
    display.setTextSize(2);   
    display.setCursor(8,14);  // center Time display
    if (CurrentTime.startsWith("0")){
      display.println(CurrentTime.substring(1,5));
    }else {
      display.setCursor(0,14);
      display.println(CurrentTime.substring(0,5));
    }
    display.setCursor(7,33); // center Time display
  }
  display.print("(" + CurrentTime.substring(6,8) + ")");
  display.display();
}
//////////////////////////////////////////////////////
// --- Pantalla 4: Forecast >>> Cielo y Nubosidad % //
//////////////////////////////////////////////////////
void previsionWeather() {
    // Alterna los datos de la previsión: Cielo y Nubosidad % 
    // y reinicia el temporizador
    nScreen = !nScreen;
    lastIntervalChange = millis();
   
    // Cabecera
    display.clearDisplay();
    display.setTextSize(pantallaGrande ? 2 : 1);
    display.setCursor(pantallaGrande ? 0 : 0, 0);
    if (!nScreen) {
      display.println(F("PREVISION:"));
    }else {
      display.println(F("NUBOSIDAD:"));
    }
    if (!pantallaGrande) display.drawLine(0, 8, 63, 8, WHITE);
  
    // Tres tramos de 3 horas
    for (int i = 0; i < FORECAST_COUNT; i++) {
      int y = pantallaGrande ? (14 + i * 15)+2 : (10 + i * 12)+2;
      display.setTextSize(pantallaGrande ? 2 : 1);
      display.setCursor(0, y);

      // Hora: "HH"
      char hora_s[3] = {0};
      if (strlen(forecast[i].hora) >= 2) strncpy(hora_s, forecast[i].hora, 2);
      else strcpy(hora_s, "--");

       // Traducir icono a texto corto
      String ic = String(forecast[i].icono);
      String iconoTxt = "-----";
  
      if      (ic.indexOf("01d") >= 0) iconoTxt = "Sol";
      else if (ic.indexOf("01n") >= 0) iconoTxt = "Despej";
      else if (ic.indexOf("02")  >= 0) iconoTxt = "Nubes";
      else if (ic.indexOf("03")  >= 0) iconoTxt = "Nublad";
      else if (ic.indexOf("04")  >= 0) iconoTxt = "Cubier";
      else if (ic.indexOf("09")  >= 0) iconoTxt = "Lluvia";
      else if (ic.indexOf("10")  >= 0) iconoTxt = "Lluvia";
      else if (ic.indexOf("11")  >= 0) iconoTxt = "Tormen";
      else if (ic.indexOf("13")  >= 0) iconoTxt = "Nieve";
      else if (ic.indexOf("50")  >= 0) iconoTxt = "Bruma";

      // Nubosidad %
      String nubes = String(forecast[i].clouds)+ "%";

      // Alterna los datos de la previsión
      display.print(hora_s);
      display.print(F("H "));
      if (!nScreen) {
        // Ejemplo: "09H Nublad"
        display.print(iconoTxt);
      }else {
        // Ejemplo: "09H N:45% "
        display.print(F("N:"));
        display.print(nubes);
      }
    }
    display.display();
  }
////////////////////////////////////////////////////////////////
// --- Pantalla 5: Forecast >>> Precipitación % y Temperatura //
////////////////////////////////////////////////////////////////
void previsionWeather2() {
    // Alterna los datos de la previsión: Precipitación % y Temperatura
    // y reinicia el temporizador
    nScreen = !nScreen;
    lastIntervalChange = millis();
   
    // Cabecera
    display.clearDisplay();
    display.setTextSize(pantallaGrande ? 2 : 1);
    display.setCursor(pantallaGrande ? 0 : 0, 0);
    if (!nScreen) {
      display.print(F("PRE.LLUVIA"));
    }else {
      display.print(F("PREVIS. "));
      display.print((char)247);
      display.print(F("C"));
    }
    if (!pantallaGrande) display.drawLine(0, 8, 63, 8, WHITE);
  
    // Tres tramos de 3 horas
    for (int i = 0; i < FORECAST_COUNT; i++) {
      int y = pantallaGrande ? (14 + i * 15)+2 : (10 + i * 12)+2;
      display.setTextSize(pantallaGrande ? 2 : 1);
      display.setCursor(0, y);

      // Hora: "HH"
      char hora_s[3] = {0};
      if (strlen(forecast[i].hora) >= 2) strncpy(hora_s, forecast[i].hora, 2);
      else strcpy(hora_s, "--");

      // Precipitación lluvia %
      String lluvia = String(forecast[i].pop);
      if (lluvia == "100") {
        lluvia = "  100%";
      }else{
        lluvia = "un " + lluvia + "%";
      }
      
      // Temperatura con 1 decimal
      float temp = forecast[i].temperatura;
      
      // Alterna los datos de la previsión
      display.print(hora_s);
      display.print(F("H "));
      if (!nScreen) {
        // Ejemplo: "09H un 23%"
        display.print(lluvia);
      }else {
        // Ejemplo: "09H 21.3ºC"
        display.print(temp, 1);
        display.print((char)247);
        display.print(F("C"));
      }
    }
    display.display();
  }
/////////////////////////////////////////////////////
void webForecast() {
  // Tres tramos de 3 horas
  PRINTS("\n>>> WebForecast <<<\n");
  for (int i = 0; i < FORECAST_COUNT; i++) {
    // Hora: "HH"
    char hora_s[3] = {0};
    if (strlen(forecast[i].hora) >= 2) strncpy(hora_s, forecast[i].hora, 2);
    else strcpy(hora_s, "--");
  
    // Precipitación lluvia %
    String lluvia = String(forecast[i].pop) + "%";

    // Nubosidad %
    String nubes = String(forecast[i].clouds)+ "%";
    
    // Temperatura con 1 decimal
    float temp = forecast[i].temperatura;
    
     // Traducir icono a texto corto
    String ic = String(forecast[i].icono);
    String iconoTxt = "-----";
  
    if      (ic.indexOf("01d") >= 0) iconoTxt = "Sol";
    else if (ic.indexOf("01n") >= 0) iconoTxt = "Despejado";
    else if (ic.indexOf("02")  >= 0) iconoTxt = "Nubes";
    else if (ic.indexOf("03")  >= 0) iconoTxt = "Nublado";
    else if (ic.indexOf("04")  >= 0) iconoTxt = "Cubierto";
    else if (ic.indexOf("09")  >= 0) iconoTxt = "Lluvia";
    else if (ic.indexOf("10")  >= 0) iconoTxt = "Lluvia";
    else if (ic.indexOf("11")  >= 0) iconoTxt = "Tormenta";
    else if (ic.indexOf("13")  >= 0) iconoTxt = "Nieve";
    else if (ic.indexOf("50")  >= 0) iconoTxt = "Bruma";
  
    String m = "00";
    if (String(hora_s) == "--") m = "--";
    String cadena = String(hora_s) + ":" + m + " [" + String(temp,1) + "ºC] " + iconoTxt;
    cadena += ", " + nubes + " de nubes y " + lluvia + " riesgo de lluvia";  
    Serial.println (cadena);
    webpage += cadena  + "<br>";
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void horasSol() {
    // Cabecera
    display.clearDisplay();
    display.setTextSize(pantallaGrande ? 2 : 1);
    display.setCursor(pantallaGrande ? 0 : 0, 0);
    display.println(F("HORAS SOL:"));
    if (!pantallaGrande) display.drawLine(0, 8, 63, 8, WHITE);

    for (int i = 0; i < 3; i++) {
      int y = pantallaGrande ? (14 + i * 15)+2 : (10 + i * 12)+2;
      display.setTextSize(pantallaGrande ? 2 : 1);
      display.setCursor(0, y);
      if (i == 0) {
        display.print(F("DIA: "));
        display.print(tiempoSol);
      }else if (i == 1) {
        display.print(F("Fin: "));
        display.print(finalSol);
      }else {
        display.print(F("Ini: "));
        display.print(inicioSol);
      }
    }
    display.display();
}
/////////////////////////////////////////////////////
void webLuzSolar() {
    // Tres tramos de 3 horas
    PRINTS("\n>>> WebLuzSolar <<<\n");

    String cadena = "Sol desde las " + inicioSol + " hasta las " + finalSol + " [" + tiempoSol + " hoy]";
    Serial.println (cadena);
    webpage += cadena  + "<br>";
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Choose your time zone from: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv 
// See below for examples
// Or, choose a time server close to you, but in most cases it's best to use pool.ntp.org to find an NTP server
// then the NTP system decides e.g. 0.pool.ntp.org, 1.pool.ntp.org as the NTP syem tries to find  the closest available servers
// EU "0.europe.pool.ntp.org"
// US "0.north-america.pool.ntp.org"
// See: https://www.ntppool.org/en/                                                           
// UK normal time is GMT, so GMT Offset is 0, for US (-5Hrs) is typically -18000, AU is typically (+8hrs) 28800
// In the UK DST is +1hr or 3600-secs, other countries may use 2hrs 7200 or 30-mins 1800 or 5.5hrs 19800 Ahead of GMT use + offset behind - offset
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
// Configura NTP y espera hasta sincronizar (ESP32/8266) //
///////////////////////////////////////////////////////////
boolean SetupTime() {
  const char* ntpServer = "es.pool.ntp.org";
  int gmtOffset_sec = 0;
  int daylightOffset_sec = 0;

  char Timezone[64];
  String displayZone = "";  // Texto legible para mostrar en Serial

  if (!T_Zone2) {
    // España peninsular (CET / CEST)
    strcpy(Timezone, "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00");
    displayZone = "UTC+1 (invierno) / UTC+2 (verano)";
    daylightOffset_sec = 7200;
  } else {
    // Corrige el signo automáticamente si zone2 es tipo "UTC+N" o "UTC-N"
    if (zone2.startsWith("UTC+")) {
      int offset = zone2.substring(4).toInt();
      snprintf(Timezone, sizeof(Timezone), "UTC-%d", offset);  // POSIX invertido
      displayZone = "UTC+" + String(offset);
    } 
    else if (zone2.startsWith("UTC-")) {
      int offset = zone2.substring(4).toInt();
      snprintf(Timezone, sizeof(Timezone), "UTC+%d", offset);  // POSIX invertido
      displayZone = "UTC-" + String(offset);
    } 
    else {
      snprintf(Timezone, sizeof(Timezone), "%s", zone2.c_str());
      displayZone = zone2;
    }
    daylightOffset_sec = 0;
  }

#ifdef ESP32
  // ----- ESP32 -----
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  setenv("TZ", Timezone, 1);
  tzset();

  Serial.printf("\n[ESP32] Zona horaria configurada (POSIX): %s", Timezone);
  Serial.printf("\n[ESP32] Hora local = %s\n", displayZone.c_str());

  const int timeout = 10000;
  unsigned long start = millis();
  struct tm timeinfo;

  while ((millis() - start) < timeout) {
    if (getLocalTime(&timeinfo)) return true;
    delay(200);
  }

  PRINTS("\n[ESP32] Error: no se pudo sincronizar con NTP");
  return false;

#else
  // ----- ESP8266 -----
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  setenv("TZ", Timezone, 1);
  tzset();

  Serial.printf("\n[ESP8266] Zona horaria configurada (POSIX): %s", Timezone);
  Serial.printf("\n[ESP8266] Hora local = %s\n", displayZone.c_str());

  const int timeout = 10000;
  unsigned long start = millis();
  time_t now = time(nullptr);

  while ((millis() - start) < timeout) {
    now = time(nullptr);
    if (now > 100000) return true;  // tiempo válido recibido
    delay(200);
  }

  PRINTS("\n[ESP8266] Error: no se pudo sincronizar con NTP");
  return false;
#endif
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Muestra por Serial la fecha y hora local actuales
//////////////////////////////////////////////////////////////////////////////
void mostrarHoraLocal() {
  struct tm timeinfo;
  time_t now;
  time(&now);

#ifdef ESP32
  if (!getLocalTime(&timeinfo)) {
    Serial.println(F("Error obteniendo hora local (ESP32)."));
    return;
  }
#else
  if (now < 100000) {
    Serial.println(F("Error obteniendo hora local (ESP8266)."));
    return;
  }
  localtime_r(&now, &timeinfo);
#endif

  char fecha[30];
  strftime(fecha, sizeof(fecha), "%d/%m/%Y %H:%M:%S", &timeinfo);

  Serial.print(F("Hora local actual: "));
  Serial.println(fecha);
}
//////////////////////////////////////////////////////////////////////////////
void display_AP_wifi() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);   
  if (pantallaGrande) {
    display.print(F("CONFI WiFi"));
    display.setCursor(0,24);   
  }else {
    display.println(F("CONFI"));
    display.setCursor(8,16);   
    display.println(F("WiFi"));
    display.setTextSize(1);   
    display.setCursor(0,32);   
  }
  display.println(F("ESP32_AP:192.168.4.1"));
  display.display();
}
//////////////////////////////////////////////////////////////
void display_flash() {
  for (int i=0; i<8; i++) {
    display.invertDisplay(true);
    display.display();
    delay(40);
    if (!pantallaGrande) delay(40);
    display.invertDisplay(false);
    display.display();
    delay(40);
    if (!pantallaGrande) delay(40);
  }
}
//////////////////////////////////////////////////////////////
void display_ip() {
  // Display OLED 
  display.clearDisplay();
  display.setTextSize(2);   
  if (pantallaGrande) {
    display.setCursor(0,0);   
    display.println(F("Entry WiFi"));
    display.setCursor(0,24);   
  }else {
    display.setCursor(4,8);   
    display.print(F("ENTRY"));
    display.setTextSize(1);   
    display.setCursor(0,26);   
  }
  display.print(F("http://"));
  display.print(WiFi.localIP());
  display.println("/");
  display.display();
}
//////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
const char *err2Str(wl_status_t code){
  switch (code){
  case WL_IDLE_STATUS:    return("IDLE");           break; // WiFi is in process of changing between statuses
  case WL_NO_SSID_AVAIL:  return("NO_SSID_AVAIL");  break; // case configured SSID cannot be reached
  case WL_CONNECTED:      return("CONNECTED");      break; // successful connection is established
  case WL_CONNECT_FAILED: return("PASSWORD_ERROR"); break; // password is incorrect
  case WL_DISCONNECTED:   return("CONNECT_FAILED"); break; // module is not configured in station mode
  default: return("??");
  }
}
////////////////////////////////////////////////////////
// Guardar configuración en EEPROM
////////////////////////////////////////////////////////
void saveConfig() {
  PRINTS("\nGuardando configuración en EEPROM...");

  // Función auxiliar para escribir texto fijo en EEPROM
  auto saveString = [](int addr, const String& data, int maxLen) {
    int i;
    for (i = 0; i < maxLen; i++) {
      if (i < data.length())
        EEPROM.write(addr + i, data[i]);
      else
        EEPROM.write(addr + i, 0);
    }
  };

  saveString(API_KEY_ADDR, apiKey, API_KEY_LEN);
  saveString(CITY_ID_ADDR, cityID, CITY_ID_LEN);
  saveString(ZONE2_ADDR, zone2, ZONE2_LEN);
  EEPROM.write(USE_ZONE2_ADDR, T_Zone2 ? 1 : 0);

  EEPROM.commit();
  PRINTS("\nConfiguración guardada correctamente.");
}
////////////////////////////////////////////////////////
// Cargar configuración desde EEPROM
// Limpia campos inválidos o con caracteres no ASCII
////////////////////////////////////////////////////////
void loadConfig() {
  auto safeReadString = [](int addr, int maxLen) {
    char data[maxLen + 1];
    bool valid = true;
    int len = 0;

    for (int i = 0; i < maxLen; i++) {
      byte b = EEPROM.read(addr + i);
      if (b == 0) break;
      // Validar: solo caracteres imprimibles y comunes
      if (b < 32 || b > 126) {
        valid = false;
        break;
      }
      data[len++] = b;
    }
    data[len] = 0;

    // Si el contenido no es válido o está vacío → limpiar
    if (!valid || len == 0) {
      for (int i = 0; i < maxLen; i++) EEPROM.write(addr + i, 0);
      EEPROM.commit();
      return String("");
    }
    return String(data);
  };

  //.................................................................

  apiKey = safeReadString(API_KEY_ADDR, API_KEY_LEN);
  cityID = safeReadString(CITY_ID_ADDR, CITY_ID_LEN);
  zone2  = safeReadString(ZONE2_ADDR, ZONE2_LEN);
  byte tzoneByte = EEPROM.read(USE_ZONE2_ADDR);
  T_Zone2 = (tzoneByte == 1);

  // Validaciones y valores por defecto
  if (zone2.length() == 0) zone2 = "UTC+0";
  zone2.trim();
  zone2.replace(" ", "+");  // corrige espacios en '+'

  PRINTS("\n=== Configuración cargada ===");
  PRINT("\nAPI Key : ",apiKey);
  PRINT("\nCity ID : ",cityID);
  PRINT("\nZone2   : ",zone2);
  PRINT("\nT_Zone2 : ",String(T_Zone2));
}
////////////////////////////////////////////////////////
// Limpiar toda la EEPROM manualmente (opcional)
////////////////////////////////////////////////////////
void clearEEPROM() {
  PRINTS("\nBorrando EEPROM...");
  for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
  EEPROM.commit();
  PRINTS("\nEEPROM borrada completamente.");
}
//////////////////////////////////////////////////////////////////////////////
// A short method of adding the same web page header to some text
//////////////////////////////////////////////////////////////////////////////
void append_webpage_header() {
  // webpage is a global variable
  webpage = ""; // A blank string variable to hold the web page
  webpage += "<!DOCTYPE html><html>"; 
  webpage += "<meta charset='UTF-8'>";   // 👈 Esto asegura que los acentos se muestren
  webpage += "<style>html { font-family:tahoma; display:inline-block; margin:0px auto; text-align:center;}";
  webpage += "#mark      {border: 5px solid #316573 ; width: 1020px;} "; 
  webpage += "#header    {background-color:#C3E0E8; width:1000px; padding:10px; color:#13414E; font-size:32px;}";
  webpage += "#section   {background-color:#E6F5F9; width:980px; padding:10px; color:#0D7693 ; font-size:20px;}";
  webpage += "#footer    {background-color:#C3E0E8; width:980px; padding:10px; color:#13414E; font-size:26px; clear:both;}";

  webpage += ".content-select select::-ms-expand {display: none;}";
  webpage += ".content-input input,.content-select select{appearance: none;-webkit-appearance: none;-moz-appearance: none;}";
  webpage += ".content-select select{background-color:#C3E0E8; width:8%; padding:10px; color:#0D7693 ; font-size:70px ; border:6px solid rgba(0,0,0,0.2) ; border-radius: 24px;}";

  webpage += ".button {box-shadow: 0px 10px 14px -7px #276873; background:linear-gradient(to bottom, #599bb3 5%, #408c99 100%);";
  webpage += "background-color:#599bb3; border-radius:8px; color:white; padding:13px 32px; display:inline-block; cursor:pointer;";
  webpage += "text-decoration:none;text-shadow:0px 1px 0px #3d768a; font-size:50px; font-weight:bold; margin:2px;}";
  webpage += ".button:hover {background:linear-gradient(to bottom, #408c99 5%, #599bb3 100%); background-color:#408c99;}";
  webpage += ".button:active {position:relative; top:1px;}";
 
  webpage += ".button2 {box-shadow: 0px 10px 14px -7px #8a2a21; background:linear-gradient(to bottom, #f24437 5%, #c62d1f 100%);";
  webpage += "background-color:#f24437; text-shadow:0px 1px 0px #810e05; }";
  webpage += ".button2:hover {background:linear-gradient(to bottom, #c62d1f 5%, #f24437 100%); background-color:#f24437;}";
  
  webpage += ".line {border: 3px solid #666; border-radius: 300px/10px; height:0px; width:80%;}";
  
  webpage += "input[type=\"text\"] {font-size:42px; width:90%; text-align:left;}";
  
  webpage += "input[type=range]{height:61px; -webkit-appearance:none;  margin:10px 0; width:65%;}";
  webpage += "input[type=range]:focus {outline:none;}";
  webpage += "input[type=range]::-webkit-slider-runnable-track {width:70%; height:30px; cursor:pointer; animate:0.2s; box-shadow: 2px 2px 5px #000000; background:#C3E0E8;border-radius:10px; border:1px solid #000000;}";
  webpage += "input[type=range]::-webkit-slider-thumb {box-shadow:3px 3px 6px #000000; border:2px solid #FFFFFF; height:50px; width:50px; border-radius:15px; background:#316573; cursor:pointer; -webkit-appearance:none; margin-top:-11.5px;}";
  webpage += "input[type=range]:focus::-webkit-slider-runnable-track {background: #C3E0E8;}";
  webpage += "</style>";
 
  webpage += "<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/animate.css/4.1.1/animate.min.css\"/>";
  webpage += "<html><head><title>NTP Weather</title>";
  webpage += "</head>";
  
  // ==========================
  // SCRIPTS JAVASCRIPT
  // ==========================
  webpage += "<script>";
  // --- Enviar API Key y City ID ---
  webpage += "function SendApiKey(){";
  webpage += "  var api = document.getElementById('apiKeyInput').value;";
  webpage += "  var city = document.getElementById('cityIDInput').value;";
  webpage += "  if(api === '' || city === ''){ alert('Por favor, introduce API Key y City ID válidos'); return; }";
  webpage += "  var request = new XMLHttpRequest();";
  webpage += "  request.open('GET', '/SAVE_API?API=' + encodeURIComponent(api) + '&CITY=' + encodeURIComponent(city), true);";
  webpage += "  request.send();";
  webpage += "  alert('Datos enviados al dispositivo');";
  webpage += "}";

  ///////////////////////////////////////////////////
  // --- Enviar Zona y Activación ---
  ///////////////////////////////////////////////////
  webpage += "function SendZoneConfig(){";
  webpage += "  var zone=document.getElementById('zoneSelect').value;";
  webpage += "  var use=document.getElementById('useZone2').checked;"; // ← devuelve true o false
  webpage += "  var x=new XMLHttpRequest();";
  webpage += "  x.open('GET','/SAVE_ZONE?ZONE='+encodeURIComponent(zone)+'&USE='+(use?1:0),true);";
  webpage += "  x.onreadystatechange=function(){";
  webpage += "    if(x.readyState==4 && x.status==200){";
  webpage += "      alert('Zona enviada:\\nZONE = '+zone+'\\nUSE = '+use);"; // ← muestra valores actuales
  webpage += "      document.open(); document.write(x.responseText); document.close();";
  webpage += "    }";
  webpage += "  };";
  webpage += "  x.send();";
  webpage += "}";
  ///////////////////////////////////////////////////

  webpage += "</script>";
  // =========================

  webpage += "<div id=\"mark\">";
  webpage += "<div id=\"header\"><h1 class=\"animate__animated animate__flash\">NTP Weather " + HWversion + "</h1>";
}
//////////////////////////////////////////////////////////////////////////////
void button_Home() {
  webpage += "<p><a href=\"\\HOME\"><type=\"button\" class=\"button\">Refresh WEB</button></a></p>";
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void NTP_Clock_home_page() {
  append_webpage_header();
  webpage += "<h3 class=\"animate__animated animate__fadeInLeft\">RTC = ";
  if (T_Zone2 == false) webpage += zone1; else webpage += zone2;

  webpage += "<br>";
  webpage += "Refresh: " + CurrentDate + " - " + CurrentTime;
  webpage += "<br>";
  webpage += weatherLoc + " = " + String(temp,1) + "ºC - Hr: " + String(humidity,0) + "%<br>";

  ///////////////////////////////////////////////////////////////////////
  // =========================================
  // Panel del clima alineado y proporcional
  // =========================================
  ///////////////////////////////////////////////////////////////////////
  webpage += "<div style='display:flex; align-items:flex-start; justify-content:center; gap:90px; flex-wrap:wrap; margin-top:30px;'>";

  // --- Icono del clima ---
  webpage += "<div style='text-align:center;'>";
  webpage += "<img id='weatherIcon' src='https://openweathermap.org/img/wn/" + weatherIcon + "@2x.png' ";
  webpage += "alt='icono' style='width:200px;height:200px;'>";
  
  webpage += "<p id='weatherDesc' style='font-size:32px; margin-top:14px; line-height:1.4; font-weight:bold;'>" + weatherDesc;
  webpage += "<br>" + String(pressure,0) + " mbar";
  webpage += "<br><b>Feels:</b> " + String(feels,1) + "ºC</p>";
  webpage += "</div>";

  // --- SVG brújula compacta y centrada verticalmente ---
  webpage += "<div style='text-align:center; margin-top:10px;'>";

  webpage += "<svg id='compass' width='180' height='180' viewBox='0 0 200 200' ";
  webpage += "style='border:3px solid #444; border-radius:50%; background:#f5f5f5; vertical-align:middle; box-shadow:0 0 10px rgba(0,0,0,0.2);'>";
  webpage += "  <defs>";
  webpage += "    <radialGradient id='windGrad' cx='50%' cy='50%' r='50%'>";
  webpage += "      <stop offset='0%' stop-color='rgba(150,200,255,0.2)'/>";
  webpage += "      <stop offset='100%' stop-color='rgba(0,0,128,0.5)'/>";
  webpage += "    </radialGradient>";
  webpage += "  </defs>";

  webpage += "  <circle id='windRose' cx='100' cy='100' r='80' fill='url(#windGrad)' opacity='0.35'/>";
  webpage += "  <circle cx='100' cy='100' r='85' fill='none' stroke='#555' stroke-width='2'/>";

  webpage += "  <text x='100' y='22' text-anchor='middle' font-size='16' fill='#222'>N</text>";
  webpage += "  <text x='180' y='106' text-anchor='middle' font-size='16' fill='#222'>E</text>";
  webpage += "  <text x='100' y='190' text-anchor='middle' font-size='16' fill='#222'>S</text>";
  webpage += "  <text x='20' y='106' text-anchor='middle' font-size='16' fill='#222'>O</text>";

  // Aguja roja
  webpage += "  <polygon id='needle' points='100,35 95,100 105,100' fill='red' stroke='black' stroke-width='1.5'/>";
  webpage += "  <circle cx='100' cy='100' r='4' fill='#333'/>";
  webpage += "</svg>";

  // Datos de viento
  webpage += "<br><br><p style='font-size:32px; font-weight:bold; margin-top:16px;'>Viento: " + String(deg) + "º<br>" + String(wind,0) + " km/h</p>";
  webpage += "</div>";

  webpage += "</div>";

  // --- Script de animación y orientación ---
  webpage += "<script>";
  webpage += "  var deg = " + String(deg) + ";";  
  webpage += "  var windSpeed = " + String(wind) + ";";  
  webpage += "  var needle = document.getElementById('needle');";
  webpage += "  var compass = document.getElementById('compass');";
  webpage += "  var rose = document.getElementById('windRose');";
  webpage += "  if(needle){";
  webpage += "    needle.style.transformOrigin = '100px 100px';";
  webpage += "    needle.style.transition = 'transform 1.5s ease-out';";
  // La aguja apunta HACIA donde va el viento, no de donde viene
  webpage += "    needle.style.transform = 'rotate(' + (deg + 180) + 'deg)';";
  webpage += "  }";

  // Rotación suave de la rosa según velocidad del viento
  webpage += "  var angle = 0;";
  webpage += "  var speed = Math.max(0.2, Math.min(windSpeed/5, 2.5));"; // velocidad limitada
  webpage += "  setInterval(function(){ angle += speed; rose.style.transform = 'rotate(' + angle + 'deg)'; rose.style.transformOrigin = '100px 100px'; }, 120);";

  // Modo oscuro si el icono es nocturno
  webpage += "  var icon = '" + weatherIcon + "';";
  webpage += "  if(icon.endsWith('n')){";
  webpage += "    compass.style.background = '#1e1e1e';";
  webpage += "    compass.style.borderColor = '#ccc';";
  webpage += "    compass.querySelectorAll('text').forEach(t => t.setAttribute('fill', '#eee'));";
  webpage += "    needle.setAttribute('fill', '#ff6666');";
  webpage += "  }";
  webpage += "</script>";

  ///////////////////////////////////////////////////////////////////////
  webpage += "<label style='font-size:12px;'>";
  webForecast();
  webpage += "</label><label style='font-size:24px;'>";
  webLuzSolar();
  webpage += "</label>";
  webpage += "</h3>";
  
  webpage += "<div id=\"section\">";
  
  button_Home();

  ///////////////////////////////////////////////////////////////////////
  webpage += "<hr class=\"line\">";
  ///////////////////////////////////////////////////////////////////////

  webpage += "<h2>&#149; Connect to: <b><a href=https://openweathermap.org/>OpenWeatherMap</a></b></h2>";

  // ==========================
  // API Key y City ID
  // ==========================
  //webpage += "<h2>Configuración de API Key & City ID</h2>";
  webpage += "<p><b>API Key:</b></p>";
  webpage += "<input type='text' id='apiKeyInput' placeholder='Tu apiKey' value='";
  webpage += apiKey; // 👉 Aquí insertamos el valor actual
  webpage += "' style='width:900px; font-size:50px;'>";

  webpage += "<p><b>City ID:</b></p>";
  webpage += "<input type='text' id='cityIDInput' placeholder='Tu cityID' value='";
  webpage += cityID; // 👉 Aquí insertamos el valor actual
  webpage += "' style='width:240px; font-size:50px;'>";

  // Botón SendApiKey()
  webpage += "<br><br><p><button type=\"button\" onclick=\"SendApiKey()\" class=\"button\" ";
  webpage += "'>Guardar API Key / City</button></p>";

  ///////////////////////////////////////////////////////////////////////
  webpage += "<hr class=\"line\">";
  ///////////////////////////////////////////////////////////////////////

  // ==========================
  // Zona Horaria
  // ==========================
  webpage += "<p>Zona horaria (UTC offset)</p>";
  webpage += "<select id='zoneSelect' style='font-size:42px;'>";
  for (int i = -12; i <= 12; i++) {
    String zoneStr = "UTC";
    zoneStr += (i >= 0 ? "+" : "");
    zoneStr += String(i);
    webpage += "<option value='" + zoneStr + "'";
    if (zone2 == zoneStr) webpage += " selected";
    webpage += ">" + zoneStr + "</option>";
  }
  webpage += "</select></p>";

  // Checkbox para T_Zone2
  webpage += "<p>Usar Zona UTC personalizada</p>";
  webpage += "<label style='font-size:28px;'>"
             "<input type='checkbox' id='useZone2' style='width:30px; height:30px; transform:scale(1.5); vertical-align:middle;' "
             + String(T_Zone2 ? "checked" : "") +
             "> Activar</label>";
  
  // Botón SendZoneConfig()
  webpage += "<br><p><button type=\"button\" onclick=\"SendZoneConfig()\" class=\"button\" ";
  webpage += "'>Guardar Zona Horaria</button></p>";


  webpage += "<p><a href=\"\\RESTART_1\"><type=\"button\" class=\"button\">RTC = ";
  webpage += zone1;
  if (T_Zone2 == false)  webpage += " #";
  webpage += "</button></a>";

  webpage += "<a href=\"\\RESTART_2\"><type=\"button\" class=\"button\">RTC = ";
  webpage += zone2;
  if (T_Zone2 == true)  webpage += " #";
  webpage += "</button></a></p>";

  webpage += "<p><a href=\"\\AVISO_RESET\"><type=\"button\" class=\"button button2\">RESET</button></a></p>";
  
  webpage += "</div>";
  end_webpage();
}
//////////////////////////////////////////////////////////////////////////////
void _reset_wifi() {
  PRINTS("\n-> RESET_WIFI");
  append_webpage_header();
  webpage += "<p><h2>New WiFi Connection</h2></p>";
  
  webpage += "<div id=\"section\">";
  webpage += "<span style='font-size:38px;text-shadow:0px 1px 0px #810e05;'";
  webpage += "<p>&#149; Connect WiFi to SSID: <b>ESP32_AP</b></p>";
  webpage += "<p>&#149; Next connect to: <b><a href=http://192.168.4.1/>http://192.168.4.1/</a></b></p>";
  webpage += "<p>&#149; Make the WiFi connection</span></p>";
  button_Home();
  webpage += "</div>";
  end_webpage();
  delay(1000);
  WiFiManager wifiManager;
  wifiManager.resetSettings();      // RESET WiFi in ESP32
  ESP.restart();
}
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
void _aviso_reset() {
  append_webpage_header();
  webpage += "<p><h2>¿ Estás seguro ?</h2></p>";
  
  webpage += "<div id=\"section\">";
  webpage += "<span style='font-size:38px;text-shadow:0px 1px 0px #810e05;'><b>[Reset CONFIG]</b> borrará toda";
  webpage += " la información almacenada en la EEPROM. <b>La estación meteorológica dejará de recibir datos.</b>";
  
  webpage += "<br><br><b>[Reset WIFI]</b> borrará toda";
  webpage += " la información de la red WiFi que estás utilizando. <b>ESP dejará de funcionar</b>, ";
  webpage += " y luego tendrás que hacer lo siguiente:";

  webpage += "<p>&#149; Conectar el WiFi de tu móvil con el SSID: <b>ESP32_AP</b></p>";
  webpage += "<p>&#149; Después conectar con: <b><a href=http://192.168.4.1/>http://192.168.4.1/</a></b></p>";
  webpage += "<p>&#149; Configurar la nueva red WiFi</span></p>";
  webpage += "</span>";
    
  button_Home();
  webpage += "<br><p><a href=\"\\RESET_CONFIG\"><type=\"button\" class=\"button button2\">Reset CONFIG</button></a>";
  webpage += "<a href=\"\\RESET_WIFI\"><type=\"button\" class=\"button button2\">Reset WiFi</button></a></p>";
  webpage += "</div>";
  end_webpage();
}
//////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
void _restart_1() {
  T_Zone2=false;
  PRINT("\n>>> SYNC Time: ",zone1);
  _restart();   
}
/////////////////////////////////////////////////////////////////
void _restart_2() {
  T_Zone2=true;
  PRINT("\n>>> SYNC Time: ",zone2);
  _restart();   
}
/////////////////////////////////////////////////////////////////
void _restart() {
  PRINTS("\n-> RESTART");
  web_reset_ESP32(); //>>> saveConfig()
  delay(100);
  ESP.restart();  // reinicio tras enviar la respuesta
}
//////////////////////////////////////////////////////////////
void web_reset_ESP32() {
  saveConfig();
  append_webpage_header();

  webpage += "<p><h2>Restarting ESP...</h2></p>";
  webpage += "<div id=\"section\">";
  webpage += "<span style='font-size:42px;text-shadow:0px 1px 0px #810e05;'>RTC Zone: <b>";
  if (T_Zone2) {
    webpage += zone2;
  }else {
    webpage += zone1;
  }
  webpage += "</b></span><br>";
  button_Home();
  webpage += "</div>";

  end_webpage();  // <-- envía la página al cliente

  PRINTS("\n>>> Página de reinicio enviada correctamente");
}
//////////////////////////////////////////////////////////////
void end_webpage(){
  webpage += "<div id=\"footer\">Copyright &copy; J_RPM 2025</div></div></html>\r\n";
  server.send(200, "text/html", webpage);
  PRINTS("\n>>> end_webpage() OK! ");
}
/////////////////////////////////////////////////////////////////
void _home() {
  PRINTS("\n-> HOME");
  NTP_Clock_home_page();
  PRINTS("\n-> SendPage");
}
/////////////////////////////////////////////////////////////////
void checkServer() {
  server.begin();  // Start the WebServer
  PRINTS("\nWebServer started");

  // Rutas principales
  server.on("/", _home);
  server.on("/HOME", _home);
  server.on("/RESTART_1", _restart_1);  
  server.on("/RESTART_2", _restart_2);  
  server.on("/AVISO_RESET", _aviso_reset);
  server.on("/RESET_WIFI", _reset_wifi);

  server.on("/RESET_CONFIG", []() {
    clearEEPROM();
    PRINTS("\n>>> EEPROM limpiada y configuración eliminada.");
    NTP_Clock_home_page();
    ESP.restart();
  });

  server.on("/SAVE_API", []() {
    if (server.hasArg("API") && server.hasArg("CITY")) {
      apiKey = server.arg("API");
      cityID = server.arg("CITY");
      saveConfig(); // Guarda ambos
      server.send(200, "text/plain", "API Key y City ID guardadas");
      PRINTS("\n>>> API Key y City ID guardadas");
      obtenerClima();
    } else {
      PRINTS("\n>>> Error: faltan parámetros");
      server.send(400, "text/plain", "Error: faltan parámetros");
    }
  });

  server.on("/SAVE_ZONE", []() {
    if (server.hasArg("ZONE") && server.hasArg("USE")) {
      String _zone2 = server.arg("ZONE");
      _zone2.replace(" ", "+");   // corrige el signo
      bool _T_Zone2 = (server.arg("USE").toInt() == 1);
      
      // Reinicia ESP sólo si cambia: T_Zone2
      if (_T_Zone2 == T_Zone2){
        if (_zone2 != zone2){
          zone2 = _zone2;
          saveConfig();
        }
        _home();
      }else {
        zone2 = _zone2;
        T_Zone2 = _T_Zone2;
        // Ejecuta la rutina de reinicio Web (envía página HTML)
        web_reset_ESP32();
    
        // ⚠ IMPORTANTE: No llamar ESP.restart() dentro de web_reset_ESP32(),
        // para dejar que el servidor envíe la página completamente.
        // Se reinicia con un temporizador no bloqueante:
        delay(100);
        ESP.restart();  // reinicio tras enviar la respuesta
      }
    } else {
      PRINTS("\n>>> Error: faltan parámetros");
      server.send(400, "text/plain", "Error: faltan parámetros");
    }
  });

}
/////////////////////////////////////////////////////////////////////////
