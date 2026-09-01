#pragma once

#include <RTClib.h>

// --------------------------------
// Központi rendszeradat-struktúra
// --------------------------------
// Melyik fűtési ág aktív
enum HeatingBranch
{
    HEATING_NONE,
    HEATING_GAS,
    HEATING_WOOD,
    HEATING_ERROR
};

// Rendszerállapotok
enum SystemState
{
    SYSTEM_OFF,
    SYSTEM_GAS,
    SYSTEM_WOOD,
    SYSTEM_FIRE_WITHOUT_CIRCULATOR,
    SYSTEM_STARTUP,
    SYSTEM_FAULT
};

// Riasztási állapotok
enum AlarmState
{
    ALARM_NONE,
    ALARM_WARNING,
    ALARM_CRITICAL
};

// --------------------------------
// Kozponti rendszeradatok
// --------------------------------

struct SystemData
{
    // Hőmérsékletek
    float gasFlowTemperature;
    float gasReturnTemperature;

    float woodFlowTemperature;
    float woodReturnTemperature;

    float woodBoilerTemperature;
    float chimneyTemperature;

    float boilerRoomTemperature;
    float boilerRoomHumidity;


    // AC állapotok
    bool gasAcPresent;
    bool woodAcPresent;


    // Feldolgozott állapotok
    bool firePresent;

    // Rendszerállapotok
    SystemState systemState;

    // Aktív fűtési ág
    HeatingBranch activeBranch;

    // Riasztási állapot
    AlarmState alarmState;

    // Idő
    DateTime timestamp;
};


// Aktuális rendszeradatok

extern SystemData systemData;