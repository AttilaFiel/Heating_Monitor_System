
#include <Arduino.h>
#include <SPI.h>

#include "config.h"
#include "system_data.h"
#include "network.h"
#include "display.h"
#include "sensors.h"
#include "outputs.h"
#include "rtc.h"
#include "watchdog.h"


// ============================================================
// Globális változók
// ============================================================

MasterData master;
LocalData local;

uint32_t lastPoll = 0;
uint32_t lastDraw = 0;


// ============================================================
// Inicializálás
// ============================================================

void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println("\\nHMS-Display boot");

    SPI.begin(
        PIN_TFT_SCK,
        PIN_TFT_MISO,
        PIN_TFT_MOSI
    );

    displayBegin();
    sensorsBegin();
    outputsBegin();
    rtcBegin();
    watchdogBegin();
    networkBegin();
    rtcSyncNtp();

    /*displayMessage(
        "HMS-Display",
        "Connecting to Wi-Fi / Master"
    );*/
}


// ============================================================
// Fő ciklus
// ============================================================

void loop() {

    // --------------------------------------------------------
    // Hálózat kezelése
    // --------------------------------------------------------

    networkLoop();


    // --------------------------------------------------------
    // Helyi érzékelők és RTC frissítése
    // --------------------------------------------------------

    sensorsUpdate(local);
    rtcUpdate(local);
    networkSetLocalData(local);


    // --------------------------------------------------------
    // Idő lekérése
    // --------------------------------------------------------

    uint32_t now = millis();


    // --------------------------------------------------------
    // Master API lekérdezése
    // --------------------------------------------------------

    if (now - lastPoll >= MASTER_POLL_MS) {
        lastPoll = now;

        if (!networkFetchMaster(master)) {
            networkLog(
                "Master API failed: " + master.lastError
            );
        }
    }


    // Master adatainak frissesség-ellenőrzése
    if (master.valid) {
        const uint32_t fetchAgeMs = now - master.lastSuccess;

        master.apiReachable =
            fetchAgeMs <= MASTER_STALE_MS;
    }


    // --------------------------------------------------------
    // Kimenetek kezelése
    // --------------------------------------------------------

    outputsUpdate(
        master.systemState,
        master.activeBranch,
        master.alarmState,
        local.dark
    );


    // --------------------------------------------------------
    // Kijelző frissítése
    // --------------------------------------------------------

    if (now - lastDraw >= 1000) {
        lastDraw = now;
        displayUpdate(master, local);
    }


    // --------------------------------------------------------
    // Watchdog és ciklus késleltetése
    // --------------------------------------------------------

    watchdogFeed();
    delay(2);
}
