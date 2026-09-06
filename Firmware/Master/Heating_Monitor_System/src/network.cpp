#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

#include "config.h"
#include "network.h"
#include "logger.h"


WiFiServer terminalServer(TERMINAL_PORT);

WiFiClient terminalClient;


bool previousWiFiConnected = false;

unsigned long wifiLastAttempt = 0;
const unsigned long WIFI_RECONNECT_INTERVAL = 10000;


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


    // OTA
    ArduinoOTA.setHostname(OTA_HOSTNAME);

    ArduinoOTA.onStart([]()
    {
        logMessage("OTA feltoltes indul!");
    });

    ArduinoOTA.onEnd([]()
    {
        logMessage("OTA feltoltes kesz!");
    });

    ArduinoOTA.begin();


    logMessage("OTA keszen all!");
    logMessage("WiFi terminal keszen all!");
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