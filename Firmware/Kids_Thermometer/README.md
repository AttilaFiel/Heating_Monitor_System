# HMS Kids Thermometer

ESP32-C3-SUPERMINI + SHT30 room thermometer. Provides read-only HTTP API, OTA, NTP local time and a Wi-Fi terminal.

## Setup
1. Copy `include/secrets.h.example` to `include/secrets.h`.
2. Enter the Wi-Fi SSID/password from the HMS Master `secrets.h`.
3. Confirm the board's I2C pins and network settings in `include/config.h`.
4. Build/upload once over USB. OTA uploads then target `192.168.1.112`.

## Endpoints
- `GET http://192.168.1.112/api/v1/status`
- `GET http://192.168.1.112/api/v1/health`

Status JSON contains `schemaVersion`, local NTP timestamp with Europe/Budapest DST, uptime, temperature, humidity, sensor validity, data age and Wi-Fi state.

## Wi-Fi terminal
Connect with a Telnet client to `192.168.1.112:23`. Commands: `status`, `help`.
The terminal is intended for a trusted LAN only; no authentication is implemented. Do not expose it to the Internet.

## Low-power/thermal notes
The sensor is sampled every 30 seconds; the ESP32 remains awake to keep HTTP/OTA/terminal responsive. This prioritizes reliable always-on API availability over deep sleep. Sensor and ESP32 are enclosed together; use the SHT30's own vented area, keep it away from the ESP32/regulator, and avoid continuous high-rate polling. The firmware does not drive a heater or control the HMS Master.

## Notes
The supplied Master API contract describes HMS-Master's schema. This standalone thermometer exposes its own minimal schema because the Master contract is specific to boiler-system data. The device uses static IP `192.168.1.112`.
