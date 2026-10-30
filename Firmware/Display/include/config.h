
#pragma once

#include <Arduino.h>


// ============================================================
// Master API konfiguráció
// ============================================================

constexpr char MASTER_HOST[] = "192.168.1.110";
constexpr char MASTER_STATUS_PATH[] = "/api/v1/status";


// ============================================================
// Display hálózati konfiguráció
// ============================================================

constexpr char DISPLAY_HOSTNAME[] = "HMS-Display";

constexpr uint32_t MASTER_POLL_MS = 5000;
constexpr uint32_t MASTER_STALE_MS = 30000;

constexpr uint32_t WIFI_RETRY_MS = 10000;

constexpr uint16_t TELNET_PORT = 23;


// ============================================================
// NTP és időzóna
// ============================================================

constexpr char NTP_SERVER[] = "pool.ntp.org";

// Hungary: fixed offset for now; adjust for DST or use POSIX TZ.
constexpr long GMT_OFFSET_SEC = 3600;
constexpr int DST_OFFSET_SEC = 3600;


// ============================================================
// TFT kijelző SPI lábak
// ============================================================

constexpr uint8_t PIN_TFT_SCK = 18;
constexpr uint8_t PIN_TFT_MISO = 19;
constexpr uint8_t PIN_TFT_MOSI = 23;

constexpr uint8_t PIN_TFT_CS = 17;
constexpr uint8_t PIN_TFT_DC = 16;
constexpr uint8_t PIN_TFT_RESET = 4;
constexpr uint8_t PIN_TFT_LED = 32;


// ============================================================
// Érintőképernyő lábak
// ============================================================

constexpr uint8_t PIN_TOUCH_CS = 27;
constexpr uint8_t PIN_TOUCH_IRQ = 34;


// ============================================================
// I2C és fényérzékelő lábak
// ============================================================

constexpr uint8_t PIN_SDA = 21;
constexpr uint8_t PIN_SCL = 22;
constexpr uint8_t PIN_LDR = 35;


// ============================================================
// RGB LED lábak
// ============================================================

constexpr uint8_t PIN_LED_R = 33;
constexpr uint8_t PIN_LED_G = 25;
constexpr uint8_t PIN_LED_B = 26;


// ============================================================
// Hangjelzők lábai
// ============================================================

constexpr uint8_t PIN_BUZZER_1 = 14;
constexpr uint8_t PIN_BUZZER_2 = 13;


// ============================================================
// Háttérvilágítás fényereje
// ============================================================

constexpr uint8_t BACKLIGHT_BRIGHTNESS_PERCENT = 80;
constexpr uint8_t BACKLIGHT_DARK_PERCENT = 5;


// ============================================================
// RGB LED fényereje
// ============================================================

constexpr uint8_t RGB_BRIGHTNESS_PERCENT = 80;
constexpr uint8_t RGB_DARK_PERCENT = 5;
constexpr uint8_t RGB_ALARM_PERCENT = 100;


// ============================================================
// Fényérzékelő küszöbértékei
// ============================================================

constexpr uint16_t PHOTO_DARK_THRESHOLD = 1400;
constexpr uint16_t PHOTO_HYSTERESIS = 150;

// ============================================================
// SHT30 hőmérséklet-korrekció
// ============================================================

// Pozitív vagy negatív korrekció Celsius-fokban.
//  0.0f  = nincs korrekció
// +1.5f  = 1,5 °C hozzáadása
// -1.5f  = 1,5 °C levonása
constexpr float SHT30_TEMPERATURE_OFFSET_C = -1.5f;


// ============================================================
// Érintőképernyő időzítése
// ============================================================

constexpr uint32_t TOUCH_DEBOUNCE_MS = 250;


// ============================================================
// Watchdog konfiguráció
// ============================================================

#define WATCHDOG_TIMEOUT_SECONDS 10
