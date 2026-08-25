#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

#include "config.h"
#include "network.h"


WiFiServer terminalServer(TERMINAL_PORT);

WiFiClient terminalClient;


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
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("WiFi kapcsolodas");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi csatlakozva!");

    Serial.print("IP cim: ");
    Serial.println(WiFi.localIP());


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
    // OTA kezelés
    ArduinoOTA.handle();


    // Új terminál kliens
    if (terminalServer.hasClient())
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