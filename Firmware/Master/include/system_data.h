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

// Riasztási okok
// Riasztási okok
enum AlarmReason
{
    ALARM_REASON_NONE,

    ALARM_REASON_BOTH_BRANCHES,
    ALARM_REASON_FIRE_WITHOUT_CIRCULATOR,
    ALARM_REASON_WOOD_BOILER_OVERHEAT,
    ALARM_REASON_STARTUP_FAULT,

    ALARM_REASON_RTC_BATTERY,
    ALARM_REASON_RTC_OSCILLATOR_STOP,

    ALARM_REASON_SD_WARNING,
    ALARM_REASON_SD_FULL,

    ALARM_REASON_STARTUP_WARNING,
    ALARM_REASON_WOOD_BOILER_WARNING,

    // Szenzorhibák
    ALARM_REASON_GAS_FLOW_SENSOR,
    ALARM_REASON_GAS_RETURN_SENSOR,
    ALARM_REASON_WOOD_FLOW_SENSOR,
    ALARM_REASON_WOOD_RETURN_SENSOR,
    ALARM_REASON_WOOD_BOILER_SENSOR,
    ALARM_REASON_CHIMNEY_SENSOR,
    ALARM_REASON_BOILER_ROOM_SENSOR
};

// --------------------------------
// Központi rendszeradatok
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

    // Szenzorhiba állapotok
    bool gasFlowSensorError;
    bool gasReturnSensorError;

    bool woodFlowSensorError;
    bool woodReturnSensorError;

    bool woodBoilerSensorError;

    bool chimneySensorError;

    bool boilerRoomSensorError;

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
    
    // Riasztási ok
    AlarmReason alarmReason;

    // Idő
    DateTime timestamp;
};

// Aktuális rendszeradatok
extern SystemData systemData;