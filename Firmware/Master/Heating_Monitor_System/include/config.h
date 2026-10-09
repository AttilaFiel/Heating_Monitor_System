#include "secrets.h"

#pragma once

// --------------------------------
// Hálózati beállítások
// --------------------------------

// Wi-fi csatlakozási adatok
// secrets.h fájlban definiálva
//#define WIFI_SSID "YOUR_WIFI_SSID"
//#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Wi-Fi terminál
#define TERMINAL_PORT 36987

// OTA
#define OTA_HOSTNAME "HMS-ESP32"

// --------------------------------
// GPIO pin definíciók
// --------------------------------

// I2C
#define I2C_SDA 21
#define I2C_SCL 22

// PCA9685
#define PCA9685_ADDRESS 0x40
#define PCA9685_PWM_FREQ 1000

// MicroSD / SPI
#define MICROSD_CS 13

#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23

// MAX6675
#define CHIMNEY_TEMPERATURE_CS 14

// DHT11
#define BOILER_ROOM_TEMPERATURE 26

// MVPDM-1PHS AC JELENLÉT ÉRZÉKELŐK
#define GAS_AC_PRESENT 32
#define WOOD_AC_PRESENT 33

// Buzzer
#define BUZZER 25

// DS18B20 hőmérséklet szenzorok
// DS18B20 OneWire
#define TEMPERATURE_SENSOR_BUS 27

// DS18B20 Szenzorok egyedi címei
#define GAS_FLOW_SENSOR       "2842FF9590250667"
#define GAS_RETURN_SENSOR     "2850D68990250673"

#define WOOD_FLOW_SENSOR      "28CE7739902506BE"
#define WOOD_RETURN_SENSOR    "2827C42D9125069D"

#define WOOD_BOILER_SENSOR    "28711D9E912506B5"


// SSD1306 KIJEZOK
// Kijelzők közös vezérlő jelei
#define DISPLAY_RESET 16
#define DISPLAY_DC 17

// Kijelzők Chip Select jelei
#define DISPLAY1_CS 5
#define DISPLAY2_CS 4

// RGB LED csatornák
#define RGB_LED_GAS 1   //Gáz
#define RGB_LED_WOOD 0  //Fa
#define RGB_LED_ESP32 2 //ESP32 státusz

// --------------------------------
// Hőmérséklet határértékek
// --------------------------------
#define FIRE_TEMPERATURE_THRESHOLD       80.0

#define WOOD_FLOW_HEATING_THRESHOLD      28.0
#define WOOD_FLOW_COOLING_THRESHOLD      22.0

#define WOOD_BOILER_OPTIMAL_TEMPERATURE  60.0
#define WOOD_BOILER_WARNING_TEMPERATURE  70.0
#define WOOD_BOILER_ALARM_TEMPERATURE    80.0

#define STARTUP_TIMEOUT                 900000UL   // 15 perc
#define WOOD_AC_SWITCH_TIMEOUT          120000UL   // 2 perc


// =========================================================
// RIASZTÁSI PRIORITÁS TESZT
// =========================================================

// Teszt engedélyezése
#define ALARM_PRIORITY_TEST_ENABLED false

// Egy tesztlépés 30 másodperc
#define ALARM_TEST_STEP_TIME 30000UL