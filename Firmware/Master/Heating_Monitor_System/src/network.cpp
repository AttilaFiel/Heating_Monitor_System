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
            html += "\">Letoltes</a>";

            html += " <a href=\"/delete?path=";
            html += filePath;
            html += "\" onclick=\"return confirm('Biztosan torolni szeretned ezt a fajlt?');\">Torles</a>";

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

    Serial.println("WiFi csatlakozas inditva...");

    previousWiFiConnected = false;
    wifiLastAttempt = millis();


    // Wi-Fi terminál szerver
    terminalServer.begin();

    Serial.print("WiFi terminal port: ");
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
                "<p>SD kartya nem erheto el.</p>"
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
                "<p>SD kartya hasznalatban van.</p>"
                "</body></html>"
            );
            return;
        }

        String html;

        html += "<html><head>";
        html += "<meta charset=\"UTF-8\">";
        html += "</head><body>";
        html += "<h1>HMS LOG</h1>";

        html += "<button onclick=\"";
        html += "if (confirm('Biztosan ujrainditod az ESP32-t?')) ";
        html += "window.location='/restart';";
        html += "\">ESP32 ujrainditasa</button>";

        html += "<br><br>";

        File logDirectory = SD.open("/log");

        if (!logDirectory)
        {
            html += "<p>/log konyvtar nem erheto el.</p>";
            html += "</body></html>";

            webServer.send(200, "text/html", html);
            return;
        }

        appendDirectoryToHtml(
            logDirectory,
            html,
            0,
            "/log"
        );

        logDirectory.close();

        html += "</body></html>";

        webServer.send(200, "text/html", html);
    });

    webServer.on("/download", []()
    {
        if (!sdCardIsAvailable())
        {
            webServer.send(
                503,
                "text/plain",
                "SD kartya nem erheto el."
            );
            return;
        }

        if (sdCardIsBusy())
        {
            webServer.send(
                503,
                "text/plain",
                "SD kartya hasznalatban van."
            );
            return;
        }

        if (!webServer.hasArg("path"))
        {
            webServer.send(
                400,
                "text/plain",
                "Hianyzik a fajl eleresi utja."
            );
            return;
        }

        String path = webServer.arg("path");

        if (!path.startsWith("/log/"))
        {
            webServer.send(
                403,
                "text/plain",
                "Tiltott fajl eleresi ut."
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
                "A fajl nem talalhato."
            );
            return;
        }

        String fileName = path.substring(
            path.lastIndexOf('/') + 1
        );

        webServer.sendHeader(
            "Content-Disposition",
            "attachment; filename=\"" + fileName + "\""
        );

        webServer.streamFile(
            file,
            "text/csv"
        );

        file.close();
    });

    webServer.on("/delete", []()
    {
        if (!sdCardIsAvailable())
        {
            webServer.send(
                503,
                "text/plain",
                "SD kartya nem erheto el."
            );
            return;
        }

        if (sdCardIsBusy())
        {
            webServer.send(
                503,
                "text/plain",
                "SD kartya hasznalatban van."
            );
            return;
        }

        if (!webServer.hasArg("path"))
        {
            webServer.send(
                400,
                "text/plain",
                "Hianyzik a fajl eleresi utja."
            );
            return;
        }

        String path = webServer.arg("path");

        if (!path.startsWith("/log/"))
        {
            webServer.send(
                403,
                "text/plain",
                "Tiltott fajl eleresi ut."
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
                "A fajl nem talalhato."
            );
            return;
        }

        file.close();

        if (!SD.remove(path))
        {
            webServer.send(
                500,
                "text/plain",
                "A fajl torlese sikertelen."
            );
            return;
        }

        webServer.send(
            200,
            "text/plain",
            "A fajl sikeresen torolve."
        );
    });

    webServer.on("/restart", []()
    {
        webServer.send(
            200,
            "text/plain",
            "ESP32 ujraindul..."
        );

        delay(200);

        ESP.restart();
    });
    webServer.begin();

    Serial.println("HTTP szerver keszen all!");

    // OTA
    ArduinoOTA.setHostname(OTA_HOSTNAME);

    ArduinoOTA.onStart([]()
    {
        otaActive = true;
        logMessage("OTA feltoltes indul!");
    });

    ArduinoOTA.onEnd([]()
    {
        otaActive = false;
        logMessage("OTA feltoltes kesz!");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
    {
        watchdogUpdate();
    });

    ArduinoOTA.begin();

    logMessage("OTA keszen all!");
    logMessage("WiFi terminal keszen all!");

    // NTP időszinkronizáció
    configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");

    logMessage("NTP idoszinkronizacio inditva!");
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
                "NTP ido: %04d-%02d-%02d %02d:%02d:%02d",
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
                "DS3231 ido: %04d-%02d-%02d %02d:%02d:%02d",
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

            logMessage("DS3231 ido NTP alapjan beallitva.");

            ntpTimeReceived = true;
        }
    }

    // Újracsatlakozási kísérlet
    if (!currentWiFiConnected &&
        millis() - wifiLastAttempt >= WIFI_RECONNECT_INTERVAL)
    {
        wifiLastAttempt = millis();

        Serial.println("WiFi ujracsatlakozasi kiserlet...");

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

            logMessage("WiFi terminal kliens csatlakozott!");

                        if (ntpTimeReceived)
            {
                char message[80];

                // NTP idő
                snprintf(
                    message,
                    sizeof(message),
                    "NTP ido: %04d-%02d-%02d %02d:%02d:%02d",
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
                    "DS3231 ido: %04d-%02d-%02d %02d:%02d:%02d",
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