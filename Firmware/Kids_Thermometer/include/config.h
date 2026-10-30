#pragma once

#include <Arduino.h>

// Network
static constexpr char DEVICE_HOSTNAME[] = "HMS-Kids_Thermometer";

// SHT30 I2C. ESP32-C3 SuperMini common default wiring; change if your board differs.
static constexpr uint8_t I2C_SDA_PIN = 0;
static constexpr uint8_t I2C_SCL_PIN = 1;
static constexpr uint8_t SHT30_ADDRESS = 0x44;

// Periods
static constexpr uint32_t SENSOR_INTERVAL_MS = 30000; // one sample every 30 s
static constexpr uint32_t NTP_SYNC_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL;
static constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 15000;
static constexpr uint32_t WDT_TIMEOUT_SECONDS = 30;

// Local time zone with automatic DST (Hungary).
static constexpr char NTP_SERVER_1[] = "pool.ntp.org";
static constexpr char NTP_SERVER_2[] = "time.google.com";
static constexpr char TZ_INFO[] = "CET-1CEST,M3.5.0,M10.5.0/3";

// ============================================================
// SHT30 hőmérséklet-korrekció
// ============================================================

// Pozitív vagy negatív korrekció Celsius-fokban.
//  0.0f  = nincs korrekció
// +1.5f  = 1,5 °C hozzáadása
// -1.5f  = 1,5 °C levonása
constexpr float SHT30_TEMPERATURE_OFFSET_C = -2.0f;
