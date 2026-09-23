#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <time.h>
#include <WebServer.h>
#include <SD.h>

#include "config.h"
#include "network.h"
#include "logger.h"
#include "rtc.h"
#include "sd_card.h"
#include "watchdog.h"
#include "system_data.h"

WiFiServer terminalServer(TERMINAL_PORT);
WebServer webServer(80);
WiFiClient terminalClient;


bool previousWiFiConnected = false;

unsigned long wifiLastAttempt = 0;
const unsigned long WIFI_RECONNECT_INTERVAL = 10000;

bool otaActive = false;

bool otaUpdateActive()
{
    return otaActive;
}

bool ntpTimeReceived = false;
struct tm ntpTime;

// Log üzenetek kiírása a Serial Monitorra
// és a Wi-Fi terminálra
void logMessage(const char* message)
{
    Serial.println(message);

    if (terminalClient && terminalClient.connected())
    {
        terminalClient.println(message);
    }
}

void appendDirectoryToHtml(
    File directory,
    String& html,
    int level,
    String currentPath)
{
    File entry = directory.openNextFile();

    while (entry)
    {
        for (int i = 0; i < level; i++)
        {
            html += "&nbsp;&nbsp;&nbsp;&nbsp;";
        }

        String entryName = entry.name();

        if (entry.isDirectory())
        {
            html += "[DIR] ";
            html += entryName;
            html += "<br>";

            String subPath = currentPath + "/" + entryName;

            appendDirectoryToHtml(
                entry,
                html,
                level + 1,
                subPath
            );
        }
        else
        {
            html += "[FILE] ";
            html += entryName;

            String filePath = currentPath + "/" + entryName;

            html += " <a href=\"/download?path=";
            html += filePath;
            html += "\">Letöltés</a>";

            html += " <a href=\"/delete?path=";
            html += filePath;
            html += "\" onclick=\"return confirm('Biztosan törölni szeretnéd ezt a fájlt?');\">Törles</a>";

            html += "<br>";
        }

        entry.close();
        entry = directory.openNextFile();
    }
}

// Wi-Fi, terminál és OTA indítása
void networkSetup()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.println("WiFi csatlakozás indítva...");

    previousWiFiConnected = false;
    wifiLastAttempt = millis();


    // Wi-Fi terminál szerver
    terminalServer.begin();

    Serial.print("WiFi terminál port: ");
    Serial.println(TERMINAL_PORT);

    // HTTP szerver
    webServer.on("/", []()
    {
        if (!sdCardIsAvailable())
        {
            webServer.send(
                200,
                "text/html",
                "<html><body>"
                "<h1>HMS LOG</h1>"
                "<p>SD kartya nem elérhető.</p>"
                "</body></html>"
            );
            return;
        }

        if (sdCardIsBusy())
        {
            webServer.send(
                200,
                "text/html",
                "<html><body>"
                "<h1>HMS LOG</h1>"
                "<p>SD kártya használatban van.</p>"
                "</body></html>"
            );
            return;
        }

        String html;

        html += "<html><head>";
        html += "<meta charset=\"UTF-8\">";
        html += "<meta http-equiv=\"Cache-Control\" content=\"no-cache\">";
        html += "<title>HMS LOG</title>";

        // Stílus
        html += "<style>";
        html += "body{font-family:Arial,sans-serif;margin:20px;}";
        html += "table{border-collapse:collapse;margin-top:10px;}";
        html += "td,th{border:1px solid #999;padding:5px 10px;text-align:left;}";
        html += "h1{margin-bottom:10px;}";
        html += "h2{margin-top:25px;}";
        html += ".value{font-weight:bold;}";
        html += "</style>";

        html += "</head><body>";

        // =========================================================
        // HMS LOG
        // =========================================================

        html += "<h1>HMS LOG</h1>";

        // =========================================================
        // ESP32 újraindítása
        // =========================================================

        html += "<button onclick=\"";
        html += "if (confirm('Biztosan újraindítod az ESP32-t?')) ";
        html += "window.location='/restart';";
        html += "\">ESP32 újraindítása</button>";

        // =========================================================
        // Státusz és adatok
        // =========================================================

        html += "<h2>Státusz és adatok</h2>";

        html += "<table>";

        html += "<tr><td>Rendszerállapot</td>"
                "<td id=\"systemState\">-</td></tr>";

        html += "<tr><td>Aktív ág</td>"
                "<td id=\"activeBranch\">-</td></tr>";

        html += "<tr><td>Riasztás</td>"
                "<td id=\"alarmState\">-</td></tr>";

        html += "<tr><td>Tűz</td>"
                "<td id=\"firePresent\">-</td></tr>";

        html += "<tr><td>Gáz AC</td>"
                "<td id=\"gasAcPresent\">-</td></tr>";

        html += "<tr><td>Fa AC</td>"
                "<td id=\"woodAcPresent\">-</td></tr>";

        html += "<tr><td>Gáz előremenő</td>"
                "<td id=\"gasFlowTemperature\">-</td></tr>";

        html += "<tr><td>Gáz visszatérő</td>"
                "<td id=\"gasReturnTemperature\">-</td></tr>";

        html += "<tr><td>Fa előremenő</td>"
                "<td id=\"woodFlowTemperature\">-</td></tr>";

        html += "<tr><td>Fa visszatérő</td>"
                "<td id=\"woodReturnTemperature\">-</td></tr>";

        html += "<tr><td>Fa kazán</td>"
                "<td id=\"woodBoilerTemperature\">-</td></tr>";

        html += "<tr><td>Kémény</td>"
                "<td id=\"chimneyTemperature\">-</td></tr>";

        html += "<tr><td>Kazánház hőmérséklet</td>"
                "<td id=\"boilerRoomTemperature\">-</td></tr>";

        html += "<tr><td>Kazánház páratartalom</td>"
                "<td id=\"boilerRoomHumidity\">-</td></tr>";

        html += "<tr><td>Idő</td>"
                "<td id=\"timestamp\">-</td></tr>";

        html += "</table>";

        // =========================================================
        // Logok
        // =========================================================

        html += "<h2>Logok</h2>";

        File logDirectory = SD.open("/log");

        if (!logDirectory)
        {
            html += "<p>/Log könyvtar nem elérhető.</p>";
        }
        else
        {
            appendDirectoryToHtml(
                logDirectory,
                html,
                0,
                "/log"
            );

            logDirectory.close();
        }

        // =========================================================
        // Státusz frissítése 5 másodpercenként
        // =========================================================

        html += "<script>";

        html += "function updateStatus(){";
        html += "fetch('/status')";
        html += ".then(response => response.json())";
        html += ".then(data => {";

        html += "document.getElementById('systemState').textContent=data.systemState;";
        html += "document.getElementById('activeBranch').textContent=data.activeBranch;";
        html += "document.getElementById('alarmState').textContent=data.alarmState;";
        html += "document.getElementById('firePresent').textContent=data.firePresent;";
        html += "document.getElementById('gasAcPresent').textContent=data.gasAcPresent;";
        html += "document.getElementById('woodAcPresent').textContent=data.woodAcPresent;";

        html += "document.getElementById('gasFlowTemperature').textContent=data.gasFlowTemperature+' °C';";
        html += "document.getElementById('gasReturnTemperature').textContent=data.gasReturnTemperature+' °C';";
        html += "document.getElementById('woodFlowTemperature').textContent=data.woodFlowTemperature+' °C';";
        html += "document.getElementById('woodReturnTemperature').textContent=data.woodReturnTemperature+' °C';";
        html += "document.getElementById('woodBoilerTemperature').textContent=data.woodBoilerTemperature+' °C';";
        html += "document.getElementById('chimneyTemperature').textContent=data.chimneyTemperature+' °C';";
        html += "document.getElementById('boilerRoomTemperature').textContent=data.boilerRoomTemperature+' °C';";
        html += "document.getElementById('boilerRoomHumidity').textContent=data.boilerRoomHumidity+' %';";

        html += "document.getElementById('timestamp').textContent=data.timestamp;";

        html += "})";
        html += ".catch(error => console.log('Státusz hiba:',error));";
        html += "}";

        html += "updateStatus();";
        html += "setInterval(updateStatus,5000);";

        html += "</script>";

        html += "</body></html>";

        webServer.send(
            200,
            "text/html",
            html
        );
    });

    webServer.on("/status", []()
    {
        String json = "{";

        json += "\"systemState\":\"";

        switch (systemData.systemState)
        {
            case SYSTEM_OFF:
                json += "Üzemen kívül";
                break;

            case SYSTEM_GAS:
                json += "Gáz";
                break;

            case SYSTEM_WOOD:
                json += "Fa";
                break;

            case SYSTEM_FIRE_WITHOUT_CIRCULATOR:
                json += "Tűz keringető nélkül";
                break;

            case SYSTEM_STARTUP:
                json += "Indítás";
                break;

            case SYSTEM_FAULT:
                json += "Hiba";
                break;

            default:
                json += "Ismeretlen";
                break;
        }

        json += "\",";

        json += "\"activeBranch\":\"";

        switch (systemData.activeBranch)
        {
            case HEATING_NONE:
                json += "Nincs";
                break;

            case HEATING_GAS:
                json += "Gáz";
                break;

            case HEATING_WOOD:
                json += "Fa";
                break;

            case HEATING_ERROR:
                json += "Hiba";
                break;

            default:
                json += "Ismeretlen";
                break;
        }

        json += "\",";

        json += "\"alarmState\":\"";

        switch (systemData.alarmState)
        {
            case ALARM_NONE:
                json += "Nincs";
                break;

            case ALARM_WARNING:
                json += "Figyelmeztetés";
                break;

            case ALARM_CRITICAL:
                json += "Kritikus";
                break;

            default:
                json += "Ismeretlen";
                break;
        }

        json += "\",";

        json += "\"firePresent\":\"";
        json += systemData.firePresent ? "IGEN" : "NEM";
        json += "\",";

        json += "\"gasAcPresent\":\"";
        json += systemData.gasAcPresent ? "BE" : "KI";
        json += "\",";

        json += "\"woodAcPresent\":\"";
        json += systemData.woodAcPresent ? "BE" : "KI";
        json += "\",";

        json += "\"gasFlowTemperature\":";
        json += String(systemData.gasFlowTemperature, 1);
        json += ",";

        json += "\"gasReturnTemperature\":";
        json += String(systemData.gasReturnTemperature, 1);
        json += ",";

        json += "\"woodFlowTemperature\":";
        json += String(systemData.woodFlowTemperature, 1);
        json += ",";

        json += "\"woodReturnTemperature\":";
        json += String(systemData.woodReturnTemperature, 1);
        json += ",";

        json += "\"woodBoilerTemperature\":";
        json += String(systemData.woodBoilerTemperature, 1);
        json += ",";

        json += "\"chimneyTemperature\":";
        json += String(systemData.chimneyTemperature, 1);
        json += ",";

        json += "\"boilerRoomTemperature\":";
        json += String(systemData.boilerRoomTemperature, 1);
        json += ",";

        json += "\"boilerRoomHumidity\":";
        json += String(systemData.boilerRoomHumidity, 1);
        json += ",";

        char timestamp[25];

        snprintf(
            timestamp,
            sizeof(timestamp),
            "%04d.%02d.%02d %02d:%02d:%02d",
            systemData.timestamp.year(),
            systemData.timestamp.month(),
            systemData.timestamp.day(),
            systemData.timestamp.hour(),
            systemData.timestamp.minute(),
            systemData.timestamp.second()
        );

        json += "\"timestamp\":\"";
        json += timestamp;
        json += "\"";

        json += "}";

        webServer.send(
            200,
            "application/json",
            json
        );
    });

    webServer.on("/download", []()
    {
        if (!sdCardIsAvailable())
        {
            webServer.send(
                503,
                "text/plain",
                "SD kártya nem elérhető."
            );
            return;
        }

        if (sdCardIsBusy())
        {
            webServer.send(
                503,
                "text/plain",
                "SD kártya használatban van."
            );
            return;
        }

        if (!webServer.hasArg("path"))
        {
            webServer.send(
                400,
                "text/plain",
                "Hiányzik a fájl elérési útvonala."
            );
            return;
        }

        String path = webServer.arg("path");

        if (!path.startsWith("/log/"))
        {
            webServer.send(
                403,
                "text/plain",
                "Tiltott fájl elérési útvonal."
            );
            return;
        }

        File file = SD.open(path, FILE_READ);

        if (!file || file.isDirectory())
        {
            if (file)
            {
                file.close();
            }

            webServer.send(
                404,
                "text/plain",
                "A fájl nem található."
            );
            return;
        }

        String fileName = path.substring(
            path.lastIndexOf('/') + 1
        );

        webServer.setContentLength(file.size());

        webServer.sendHeader(
            "Content-Disposition",
            "attachment; filename=\"" + fileName + "\""
        );

        webServer.send(
            200,
            "text/csv",
            ""
        );

        uint8_t buffer[4096];

        while (file.available())
        {
            size_t bytesRead = file.read(
                buffer,
                sizeof(buffer)
            );

            if (bytesRead > 0)
            {
                webServer.client().write(
                    buffer,
                    bytesRead
                );
            }

            watchdogUpdate();
            yield();
        }

        file.close();
    });

    webServer.on("/delete", []()
    {
        if (!sdCardIsAvailable())
        {
            webServer.send(
                503,
                "text/plain",
                "SD kártya nem elérhető."
            );
            return;
        }

        if (sdCardIsBusy())
        {
            webServer.send(
                503,
                "text/plain",
                "SD kártya használatban van."
            );
            return;
        }

        if (!webServer.hasArg("path"))
        {
            webServer.send(
                400,
                "text/plain",
                "Hiányzik a fájl elérési útvonala."
            );
            return;
        }

        String path = webServer.arg("path");

        if (!path.startsWith("/log/"))
        {
            webServer.send(
                403,
                "text/plain",
                "Tiltott fájl elérési útvonal."
            );
            return;
        }

        File file = SD.open(path, FILE_READ);

        if (!file || file.isDirectory())
        {
            if (file)
            {
                file.close();
            }

            webServer.send(
                404,
                "text/plain",
                "A fájl nem található."
            );
            return;
        }

        file.close();

        if (!SD.remove(path))
        {
            webServer.send(
                500,
                "text/plain",
                "A fájl törlése sikertelen."
            );
            return;
        }

        webServer.send(
            200,
            "text/plain",
            "A fájl sikeresen törölve."
        );
    });

    webServer.on("/restart", []()
    {
        webServer.send(
            200,
            "text/plain",
            "ESP32 újraindul..."
        );

        delay(200);

        ESP.restart();
    });
    webServer.begin();

    Serial.println("HTTP szerver készen áll!");

    // OTA
    ArduinoOTA.setHostname(OTA_HOSTNAME);

    ArduinoOTA.onStart([]()
    {
        otaActive = true;
        logMessage("OTA feltöltés indul!");
    });

    ArduinoOTA.onEnd([]()
    {
        otaActive = false;
        logMessage("OTA feltöltés kész!");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
    {
        watchdogUpdate();
    });

    ArduinoOTA.begin();

    logMessage("OTA készen áll!");
    logMessage("WiFi terminál készen áll!");

    // NTP időszinkronizáció
    configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");

    logMessage("NTP időszinkronizáció indítva!");
}


// Folyamatos hálózati feladatok
void networkLoop()
{
    bool currentWiFiConnected =
        WiFi.status() == WL_CONNECTED;


    // WiFi állapotváltozás
    if (currentWiFiConnected != previousWiFiConnected)
    {
        if (currentWiFiConnected)
        {
            Serial.println("WiFi csatlakozva!");
            Serial.print("IP cim: ");
            Serial.println(WiFi.localIP());

            logEvent(
                "ESP32",
                "WIFI_CONNECTED",
                "",
                ""
            );
        }
        else
        {
            Serial.println("WiFi kapcsolat megszakadt!");

            logEvent(
                "ESP32",
                "WIFI_DISCONNECTED",
                "",
                ""
            );
        }

        previousWiFiConnected = currentWiFiConnected;
    }


    // OTA kezelés
    ArduinoOTA.handle();

    // HTTP szerver kezelés
    webServer.handleClient();

    // NTP idő ellenőrzése
    if (!ntpTimeReceived)
    {
        if (getLocalTime(&ntpTime, 100))
        {
            char message[80];

            // NTP idő
            snprintf(
                message,
                sizeof(message),
                "NTP idő: %04d-%02d-%02d %02d:%02d:%02d",
                ntpTime.tm_year + 1900,
                ntpTime.tm_mon + 1,
                ntpTime.tm_mday,
                ntpTime.tm_hour,
                ntpTime.tm_min,
                ntpTime.tm_sec
            );

            logMessage(message);

            // DS3231 aktuális idő
            DateTime rtcNow = rtc.now();

            snprintf(
                message,
                sizeof(message),
                "DS3231 idő: %04d-%02d-%02d %02d:%02d:%02d",
                rtcNow.year(),
                rtcNow.month(),
                rtcNow.day(),
                rtcNow.hour(),
                rtcNow.minute(),
                rtcNow.second()
            );

            logMessage(message);

            // DS3231 beállítása NTP időre
            DateTime ntpDateTime(
                ntpTime.tm_year + 1900,
                ntpTime.tm_mon + 1,
                ntpTime.tm_mday,
                ntpTime.tm_hour,
                ntpTime.tm_min,
                ntpTime.tm_sec
            );

            rtc.adjust(ntpDateTime);

            logMessage("DS3231 idő NTP alapjan beállitva.");

            ntpTimeReceived = true;
        }
    }

    // Újracsatlakozási kísérlet
    if (!currentWiFiConnected &&
        millis() - wifiLastAttempt >= WIFI_RECONNECT_INTERVAL)
    {
        wifiLastAttempt = millis();

        Serial.println("WiFi újracsatlakozási kísérlet...");

        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }


    // Új terminál kliens
    if (currentWiFiConnected &&
        terminalServer.hasClient())
    {
        if (!terminalClient || !terminalClient.connected())
        {
            terminalClient = terminalServer.available();

            logMessage("WiFi terminál kliens csatlakozott!");

                        if (ntpTimeReceived)
            {
                char message[80];

                // NTP idő
                snprintf(
                    message,
                    sizeof(message),
                    "NTP idő: %04d.%02d.%02d %02d:%02d:%02d",
                    ntpTime.tm_year + 1900,
                    ntpTime.tm_mon + 1,
                    ntpTime.tm_mday,
                    ntpTime.tm_hour,
                    ntpTime.tm_min,
                    ntpTime.tm_sec
                );

                terminalClient.println(message);

                // DS3231 aktuális idő
                DateTime rtcNow = rtc.now();

                snprintf(
                    message,
                    sizeof(message),
                    "DS3231 idő: %04d.%02d.%02d %02d:%02d:%02d",
                    rtcNow.year(),
                    rtcNow.month(),
                    rtcNow.day(),
                    rtcNow.hour(),
                    rtcNow.minute(),
                    rtcNow.second()
                );

                terminalClient.println(message);
            }
        }
        else
        {
            // Csak egy kliens engedélyezett
            WiFiClient newClient = terminalServer.available();

            newClient.stop();
        }
    }
}


bool isTerminalConnected()
{
    return terminalClient && terminalClient.connected();
}