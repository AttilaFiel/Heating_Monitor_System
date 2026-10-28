
#include "network.h"

#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

#include "config.h"
#include "secrets.h"


// ============================================================
// Globális objektumok és változók
// ============================================================

static WebServer server(80);

static WiFiServer telnet(TELNET_PORT);
static WiFiClient telnetClient;

static bool telnetStarted = false;
static bool otaStarted = false;

static uint32_t lastRetry = 0;

static MasterData *lastMaster = nullptr;
static LocalData localCopy;


// ============================================================
// Helyi adatok frissítése
// ============================================================

void networkSetLocalData(const LocalData &data) {
    localCopy = data;
}


// ============================================================
// Naplózás
// ============================================================

void networkLog(const String &s) {
    Serial.println(s);

    if (telnetClient && telnetClient.connected()) {
        telnetClient.println(s);
    }
}


// ============================================================
// HTTP státusz végpont
// ============================================================

static void sendStatus() {
    JsonDocument d;

    d["schemaVersion"] = 1;
    d["device"] = "HMS-Display";
    d["wifiConnected"] = WiFi.status() == WL_CONNECTED;

    d["ip"] = WiFi.localIP().toString();
    d["uptimeSeconds"] = millis() / 1000;

    d["masterReachable"] =
        lastMaster && lastMaster->apiReachable;

    d["masterSystemState"] =
        lastMaster ? lastMaster->systemState : "UNKNOWN";

    d["masterAlarmState"] =
        lastMaster ? lastMaster->alarmState : "UNKNOWN";

    d["masterAlarmReason"] =
        lastMaster ? lastMaster->alarmReason : "UNKNOWN";

    d["sht30Valid"] = localCopy.shtValid;

    if (localCopy.shtValid) {
        d["sht30"]["temperatureC"] = localCopy.temp;
        d["sht30"]["humidityRH"] = localCopy.humidity;
    } else {
        d["sht30"]["temperatureC"] = nullptr;
        d["sht30"]["humidityRH"] = nullptr;
    }

    d["dateTime"] = localCopy.dateTime;
    d["rtcAvailable"] = localCopy.rtcAvailable;
    d["rtcTimeValid"] = localCopy.rtcValid;
    d["photoAdc"] = localCopy.photoAdc;
    d["dark"] = localCopy.dark;

    String out;
    serializeJsonPretty(d, out);

    server.send(200, "application/json", out);
}


// ============================================================
// Hálózat inicializálása
// ============================================================

void networkBegin() {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(DISPLAY_HOSTNAME);

    WiFi.config(
        IPAddress(192, 168, 1, 111),
        IPAddress(192, 168, 1, 1),
        IPAddress(255, 255, 255, 0),
        IPAddress(192, 168, 1, 1)
    );

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    lastRetry = millis();


    // --------------------------------------------------------
    // Főoldal
    // --------------------------------------------------------


    server.on("/", HTTP_GET, [] {
        server.send(
            200,
            "text/html",
            "<meta charset='utf-8'>"
            "<h1>HMS-Display</h1>"
            "<pre id='d'>Loading</pre>"
            "<button onclick=\"fetch('/api/v1/restart',{method:'POST'})\">"
            "Restart Display"
            "</button>"
            "<script>"
            "async function u(){"
                "let r=await fetch('/api/v1/status');"
                "d.textContent=JSON.stringify(await r.json(),null,2)"
            "}"
            "u();"
            "setInterval(u,5000)"
            "</script>"
        );
    });



    // --------------------------------------------------------
    // API végpontok
    // --------------------------------------------------------

    server.on("/api/v1/status", HTTP_GET, sendStatus);

    server.on("/api/v1/health", HTTP_GET, [] {
        server.send(
            200,
            "application/json",
            "{\\\"schemaVersion\\\":1,\\\"api\\\":\\\"ok\\\"}"
        );
    });

    server.on("/api/v1/restart", HTTP_POST, [] {
        server.send(
            202,
            "application/json",
            "{\\\"accepted\\\":true}"
        );

        delay(100);
        ESP.restart();
    });


    // --------------------------------------------------------
    // Ismeretlen végpont kezelése
    // --------------------------------------------------------

    server.onNotFound([] {
        server.send(
            404,
            "application/json",
            "{\\\"error\\\":\\\"not_found\\\"}"
        );
    });

    server.begin();

    networkLog("Display HTTP server started.");
}


// ============================================================
// Hálózati főciklus
// ============================================================

void networkLoop() {
    server.handleClient();

    if (WiFi.status() == WL_CONNECTED) {

        // ----------------------------------------------------
        // Telnet szerver indítása
        // ----------------------------------------------------

        if (!telnetStarted) {
            telnet.begin();
            telnet.setNoDelay(true);
            telnetStarted = true;

            networkLog(
                "Telnet TCP 23 ready; IP " +
                WiFi.localIP().toString()
            );
        }


        // ----------------------------------------------------
        // Telnet kliens kezelése
        // ----------------------------------------------------

        if (!telnetClient || !telnetClient.connected()) {
            WiFiClient c = telnet.available();

            if (c) {
                if (telnetClient) {
                    telnetClient.stop();
                }

                telnetClient = c;
                telnetClient.println("HMS-Display Wi-Fi terminal");
            }
        }


        // ----------------------------------------------------
        // OTA inicializálása
        // ----------------------------------------------------


        if (!otaStarted) {
            ArduinoOTA.setHostname(DISPLAY_HOSTNAME);

            ArduinoOTA.onStart([] {
                watchdogFeed();
                networkLog("OTA started");
            });

            ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
                watchdogFeed();
            });

            ArduinoOTA.onEnd([] {
                watchdogFeed();
                networkLog("OTA complete");
            });

            ArduinoOTA.onError([](ota_error_t e) {
                watchdogFeed();
                networkLog("OTA error " + String((int)e));
            });

            ArduinoOTA.begin();

            otaStarted = true;

            networkLog("OTA ready");
        }



        // ----------------------------------------------------
        // OTA kezelése
        // ----------------------------------------------------

        ArduinoOTA.handle();

    } else {

        // ----------------------------------------------------
        // Wi-Fi újracsatlakozás
        // ----------------------------------------------------

        otaStarted = false;

        if (millis() - lastRetry > WIFI_RETRY_MS) {
            lastRetry = millis();

            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        }
    }
}


// ============================================================
// Master API lekérdezése
// ============================================================

bool networkFetchMaster(MasterData &d) {
    lastMaster = &d;


    // --------------------------------------------------------
    // Wi-Fi kapcsolat ellenőrzése
    // --------------------------------------------------------

    if (WiFi.status() != WL_CONNECTED) {
        d.apiReachable = false;
        d.lastError = "Display Wi-Fi disconnected";

        return false;
    }


    // --------------------------------------------------------
    // HTTP kapcsolat inicializálása
    // --------------------------------------------------------

    HTTPClient h;

    String url = String("http://") +
                 MASTER_HOST +
                 MASTER_STATUS_PATH;

    h.setTimeout(1800);

    if (!h.begin(url)) {
        d.lastError = "HTTP begin failed";
        d.apiReachable = false;

        return false;
    }


    // --------------------------------------------------------
    // HTTP kérés
    // --------------------------------------------------------

    int code = h.GET();

    if (code != 200) {
        d.lastError = "HTTP status " + String(code);

        h.end();
        d.apiReachable = false;

        return false;
    }


    // --------------------------------------------------------
    // JSON válasz feldolgozása
    // --------------------------------------------------------

    JsonDocument j;

    auto err = deserializeJson(j, h.getStream());

    h.end();

    if (err) {
        d.lastError = String("JSON: ") + err.c_str();
        d.apiReachable = false;

        return false;
    }


    // --------------------------------------------------------
    // Általános rendszeradatok
    // --------------------------------------------------------

    d.valid = true;
    d.apiReachable = true;
    d.lastSuccess = millis();
    d.lastError = "";

    d.timestamp = j["timestamp"] | "";

    d.dataAgeMs =
        j["dataAgeMs"].is<uint32_t>()
            ? j["dataAgeMs"].as<uint32_t>()
            : UINT32_MAX;


    // --------------------------------------------------------
    // Rendszerállapot és riasztások
    // --------------------------------------------------------

    d.systemState = j["systemState"] | "UNKNOWN";
    d.activeBranch = j["activeBranch"] | "UNKNOWN";
    d.alarmState = j["alarmState"] | "UNKNOWN";
    d.alarmReason = j["alarmReason"] | "UNKNOWN";


    // --------------------------------------------------------
    // Fűtési ágak állapotai
    // --------------------------------------------------------

    d.firePresent = j["firePresent"] | false;
    d.gasAcPresent = j["gasAcPresent"] | false;
    d.woodAcPresent = j["woodAcPresent"] | false;


    // --------------------------------------------------------
    // Hőmérséklet- és páratartalom-adatok
    // --------------------------------------------------------

    JsonObjectConst m =
        j["measurements"].as<JsonObjectConst>();

    d.m.gasFlow =
        m["gasFlowTemperature"].is<float>()
            ? m["gasFlowTemperature"].as<float>()
            : NAN;

    d.m.gasReturn =
        m["gasReturnTemperature"].is<float>()
            ? m["gasReturnTemperature"].as<float>()
            : NAN;

    d.m.woodFlow =
        m["woodFlowTemperature"].is<float>()
            ? m["woodFlowTemperature"].as<float>()
            : NAN;

    d.m.woodReturn =
        m["woodReturnTemperature"].is<float>()
            ? m["woodReturnTemperature"].as<float>()
            : NAN;

    d.m.woodBoiler =
        m["woodBoilerTemperature"].is<float>()
            ? m["woodBoilerTemperature"].as<float>()
            : NAN;

    d.m.chimney =
        m["chimneyTemperature"].is<float>()
            ? m["chimneyTemperature"].as<float>()
            : NAN;

    d.m.roomTemp =
        m["boilerRoomTemperature"].is<float>()
            ? m["boilerRoomTemperature"].as<float>()
            : NAN;

    d.m.roomHumidity =
        m["boilerRoomHumidity"].is<float>()
            ? m["boilerRoomHumidity"].as<float>()
            : NAN;


    // --------------------------------------------------------
    // Szenzorhibák
    // --------------------------------------------------------

    JsonObjectConst e =
        j["sensorErrors"].as<JsonObjectConst>();

    d.gasFlowErr = e["gasFlowSensorError"] | false;
    d.gasReturnErr = e["gasReturnSensorError"] | false;
    d.woodFlowErr = e["woodFlowSensorError"] | false;
    d.woodReturnErr = e["woodReturnSensorError"] | false;
    d.woodBoilerErr = e["woodBoilerSensorError"] | false;
    d.chimneyErr = e["chimneySensorError"] | false;
    d.roomErr = e["boilerRoomSensorError"] | false;


    // --------------------------------------------------------
    // Sikeres feldolgozás
    // --------------------------------------------------------

    return true;
}
