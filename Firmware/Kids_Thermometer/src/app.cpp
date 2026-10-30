#include "app.h"
#include "config.h"
#include "secrets.h"

#include <WiFi.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <esp_task_wdt.h>
#include <time.h>

static Adafruit_SHT31 sht30;
static WebServer server(80);
static WiFiServer terminalServer(23);
static WiFiClient terminalClient;

static float temperatureC = NAN;
static float humidityRH = NAN;
static bool sensorOk = false;
static bool timeValid = false;
static uint32_t lastSensorMs = 0;
static uint32_t lastNtpSyncMs = 0;
static uint32_t lastWifiAttemptMs = 0;
static uint32_t lastTerminalMs = 0;

static String isoLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10)) return "";
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S%z", &timeinfo);
  String value(buf);
  if (value.length() >= 5) value = value.substring(0, value.length()-2) + ":" + value.substring(value.length()-2);
  return value;
}

static bool updateClockIfDue(bool force=false) {
  if (!force && millis() - lastNtpSyncMs < NTP_SYNC_INTERVAL_MS) return timeValid;
  configTzTime(TZ_INFO, NTP_SERVER_1, NTP_SERVER_2);
  lastNtpSyncMs = millis();
  struct tm timeinfo;
  timeValid = getLocalTime(&timeinfo, 10000);
  return timeValid;
}

static void jsonStatus() {
  String timestamp = isoLocalTime();
  String json = "{\"schemaVersion\":1,\"timestamp\":";
  json += timestamp.length() ? "\"" + timestamp + "\"" : "null";
  json += ",\"uptimeSeconds\":" + String(millis()/1000);
  json += ",\"temperatureC\":";
  json += sensorOk ? String(temperatureC, 2) : "null";
  json += ",\"humidityRH\":";
  json += sensorOk ? String(humidityRH, 2) : "null";
  json += ",\"sensorOk\":" + String(sensorOk ? "true":"false");
  json += ",\"dataAgeMs\":" + String(millis()-lastSensorMs);
  json += ",\"wifiConnected\":" + String(WiFi.status()==WL_CONNECTED ? "true":"false");
  json += "}";
  server.send(200, "application/json", json);
}

static void setupHttp() {
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/plain", "HMS Kids Thermometer. GET /api/v1/status or /api/v1/health");
  });
  server.on("/api/v1/status", HTTP_GET, jsonStatus);
  server.on("/api/v1/health", HTTP_GET, []() {
    server.send(200, "application/json", "{\"schemaVersion\":1,\"api\":\"ok\",\"uptimeSeconds\":" + String(millis()/1000) + "}");
  });
  server.onNotFound([]() {
    server.send(404, "application/json", "{\"error\":\"not_found\"}");
  });
  server.begin();
}

static void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(DEVICE_HOSTNAME);
  //WiFi.config(DEVICE_IP, GATEWAY_IP, SUBNET_MASK, DNS_IP);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWifiAttemptMs = millis();
}

static void setupOTA() {
  ArduinoOTA.setHostname(DEVICE_HOSTNAME);
#ifdef OTA_PASSWORD
  if (strlen(OTA_PASSWORD) > 0) ArduinoOTA.setPassword(OTA_PASSWORD);
#endif
  ArduinoOTA.begin();
}

static void sampleSensor() {
  float t = sht30.readTemperature();
  float h = sht30.readHumidity();
  if (!isnan(t) && !isnan(h) && t > -40.0f && t < 125.0f && h >= 0.0f && h <= 100.0f) {
    temperatureC = t + SHT30_TEMPERATURE_OFFSET_C;
    humidityRH = h;
    sensorOk = true;
  } else {
    sensorOk = false;
  }
  lastSensorMs = millis();
}

static void terminalPrint(const String& line) {
  if (terminalClient && terminalClient.connected()) terminalClient.println(line);
  Serial.println(line);
}

static void handleTerminal() {
  if (!terminalClient || !terminalClient.connected()) {
    WiFiClient candidate = terminalServer.available();
    if (candidate) {
      terminalClient.stop();
      terminalClient = candidate;
      terminalClient.println("HMS Kids Thermometer Wi-Fi terminal");
      terminalClient.println("Commands: status, help");
    }
  }
  if (terminalClient && terminalClient.connected() && terminalClient.available()) {
    String cmd = terminalClient.readStringUntil('\n');
    cmd.trim();
    if (cmd == "status") {
      terminalPrint("IP: " + WiFi.localIP().toString());
      terminalPrint("Sensor: " + String(sensorOk ? "OK" : "ERROR"));
      terminalPrint("Temperature C: " + (sensorOk ? String(temperatureC,2) : String("null")));
      terminalPrint("Humidity RH: " + (sensorOk ? String(humidityRH,2) : String("null")));
      terminalPrint("Time: " + (timeValid ? isoLocalTime() : String("not synced")));
    } else {
      terminalPrint("Commands: status, help");
    }
  }
}

void appSetup() {
  Serial.begin(115200);
  delay(50);

  // Feed the watchdog from the normal main loop; never feed it from an ISR.
  esp_task_wdt_init(WDT_TIMEOUT_SECONDS, true);
  esp_task_wdt_add(NULL);
  Serial.println("APPSETUP 3");

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  sht30.begin(SHT30_ADDRESS);

  setupWifi();
  setupHttp();
  setupOTA();
  terminalServer.begin();
  terminalServer.setNoDelay(true);

  Serial.println("HMS Kids Thermometer starting");
}

void appLoop() {
  // Watchdog is fed during regular firmware execution and OTA processing.
  esp_task_wdt_reset();

  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
    ArduinoOTA.handle();
    handleTerminal();
    updateClockIfDue();
  } else if (millis() - lastWifiAttemptMs >= WIFI_RETRY_INTERVAL_MS) {
    lastWifiAttemptMs = millis();
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }

  if (millis() - lastSensorMs >= SENSOR_INTERVAL_MS) sampleSensor();

  delay(2);
}
