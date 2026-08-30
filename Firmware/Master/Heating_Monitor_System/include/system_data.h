#pragma once

#include <RTClib.h>

// --------------------------------
// Központi rendszeradat-struktúra, melyik fűtési ág aktív
// --------------------------------
enum HeatingBranch
{
    HEATING_NONE,
    HEATING_GAS,
    HEATING_WOOD,
    HEATING_ERROR
};

enum SystemState
{
    SYSTEM_OFF,
    SYSTEM_GAS,
    SYSTEM_WOOD,
    SYSTEM_FIRE_WITHOUT_CIRCULATOR,
    SYSTEM_STARTUP,
    SYSTEM_FAULT
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

    SystemState systemState;


    // Aktív fűtési ág

    HeatingBranch activeBranch;


    // Idő

    DateTime timestamp;
};


// Aktuális rendszeradatok

extern SystemData systemData;