
#pragma once

#include <Arduino.h>


// ============================================================
// Mért adatok
// ============================================================

struct Measurements {
    float gasFlow = NAN;
    float gasReturn = NAN;

    float woodFlow = NAN;
    float woodReturn = NAN;

    float woodBoiler = NAN;
    float chimney = NAN;

    float roomTemp = NAN;
    float roomHumidity = NAN;
};


// ============================================================
// Master rendszeradatai
// ============================================================

struct MasterData {

    // Általános állapot
    bool valid = false;
    bool apiReachable = false;

    // Tűz és fűtési ágak állapota
    bool firePresent = false;
    bool gasAcPresent = false;
    bool woodAcPresent = false;


    // Időadatok
    uint32_t lastSuccess = 0;
    uint32_t dataAgeMs = UINT32_MAX;

    String timestamp;


    // Rendszerállapot és riasztások
    String systemState = "UNKNOWN";
    String activeBranch = "UNKNOWN";

    String alarmState = "UNKNOWN";
    String alarmReason = "UNKNOWN";

    String lastError = "No data yet";


    // Hőmérsékletek és páratartalom
    Measurements m;


    // Szenzorhibák
    bool gasFlowErr = false;
    bool gasReturnErr = false;

    bool woodFlowErr = false;
    bool woodReturnErr = false;

    bool woodBoilerErr = false;
    bool chimneyErr = false;
    bool roomErr = false;
};


// ============================================================
// Display helyi adatai
// ============================================================

struct LocalData {

    // SHT30 mérési adatok
    float temp = NAN;
    float humidity = NAN;

    // Szenzorok és RTC állapota
    bool shtValid = false;
    bool rtcAvailable = false;
    bool rtcValid = false;

    // Környezeti fényviszonyok
    bool dark = false;
    uint16_t photoAdc = 0;

    // Dátum és idő
    String dateTime;
};
