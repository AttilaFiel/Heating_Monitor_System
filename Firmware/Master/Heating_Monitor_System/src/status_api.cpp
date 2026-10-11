#include <Arduino.h>
#include <WiFi.h>
#include <math.h>
#include <stdio.h>

#include "system_data.h"
#include "status_api.h"
#include "rtc.h"
#include "sd_card.h"

// A data_manager.cpp biztosítja. false, amíg legalább egy adatfrissítési
// ciklus nem futott le.
extern bool getDataAgeMs(unsigned long &ageMs);

namespace
{
    const char* systemStateApiName(SystemState state)
    {
        switch (state)
        {
            case SYSTEM_OFF: return "SYSTEM_OFF";
            case SYSTEM_GAS: return "SYSTEM_GAS";
            case SYSTEM_WOOD: return "SYSTEM_WOOD";
            case SYSTEM_FIRE_WITHOUT_CIRCULATOR:
                return "SYSTEM_FIRE_WITHOUT_CIRCULATOR";
            case SYSTEM_STARTUP: return "SYSTEM_STARTUP";
            case SYSTEM_FAULT: return "SYSTEM_FAULT";
            default: return "UNKNOWN";
        }
    }

    const char* heatingBranchApiName(HeatingBranch branch)
    {
        switch (branch)
        {
            case HEATING_NONE: return "HEATING_NONE";
            case HEATING_GAS: return "HEATING_GAS";
            case HEATING_WOOD: return "HEATING_WOOD";
            case HEATING_ERROR: return "HEATING_ERROR";
            default: return "UNKNOWN";
        }
    }

    const char* alarmStateApiName(AlarmState state)
    {
        switch (state)
        {
            case ALARM_NONE: return "ALARM_NONE";
            case ALARM_WARNING: return "ALARM_WARNING";
            case ALARM_CRITICAL: return "ALARM_CRITICAL";
            default: return "UNKNOWN";
        }
    }

    const char* alarmReasonApiName(AlarmReason reason)
    {
        switch (reason)
        {
            case ALARM_REASON_NONE: return "ALARM_REASON_NONE";
            case ALARM_REASON_BOTH_BRANCHES: return "ALARM_REASON_BOTH_BRANCHES";
            case ALARM_REASON_FIRE_WITHOUT_CIRCULATOR:
                return "ALARM_REASON_FIRE_WITHOUT_CIRCULATOR";
            case ALARM_REASON_WOOD_BOILER_OVERHEAT:
                return "ALARM_REASON_WOOD_BOILER_OVERHEAT";
            case ALARM_REASON_STARTUP_FAULT: return "ALARM_REASON_STARTUP_FAULT";
            case ALARM_REASON_RTC_BATTERY: return "ALARM_REASON_RTC_BATTERY";
            case ALARM_REASON_RTC_OSCILLATOR_STOP:
                return "ALARM_REASON_RTC_OSCILLATOR_STOP";
            case ALARM_REASON_SD_WARNING: return "ALARM_REASON_SD_WARNING";
            case ALARM_REASON_SD_FULL: return "ALARM_REASON_SD_FULL";
            case ALARM_REASON_STARTUP_WARNING: return "ALARM_REASON_STARTUP_WARNING";
            case ALARM_REASON_WOOD_BOILER_WARNING:
                return "ALARM_REASON_WOOD_BOILER_WARNING";
            case ALARM_REASON_GAS_FLOW_SENSOR: return "ALARM_REASON_GAS_FLOW_SENSOR";
            case ALARM_REASON_GAS_RETURN_SENSOR: return "ALARM_REASON_GAS_RETURN_SENSOR";
            case ALARM_REASON_WOOD_FLOW_SENSOR: return "ALARM_REASON_WOOD_FLOW_SENSOR";
            case ALARM_REASON_WOOD_RETURN_SENSOR: return "ALARM_REASON_WOOD_RETURN_SENSOR";
            case ALARM_REASON_WOOD_BOILER_SENSOR: return "ALARM_REASON_WOOD_BOILER_SENSOR";
            case ALARM_REASON_CHIMNEY_SENSOR: return "ALARM_REASON_CHIMNEY_SENSOR";
            case ALARM_REASON_BOILER_ROOM_SENSOR: return "ALARM_REASON_BOILER_ROOM_SENSOR";
            default: return "UNKNOWN";
        }
    }

    void appendMeasurement(
        String& json,
        const char* name,
        float value,
        bool sensorError)
    {
        json += "\"";
        json += name;
        json += "\":";

        if (sensorError || !isfinite(value))
        {
            json += "null";
        }
        else
        {
            json += String(value, 1);
        }
    }

    void appendBoolean(String& json, const char* name, bool value)
    {
        json += "\"";
        json += name;
        json += "\":";
        json += value ? "true" : "false";
    }

    void appendTimestamp(String& json)
    {
        char timestamp[25];
        snprintf(
            timestamp,
            sizeof(timestamp),
            "%04d-%02d-%02dT%02d:%02d:%02d",
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
    }
}

String buildLegacyStatusJson()
{
    // A régi /status végpont mezőneveit és lokalizált értékeit szándékosan
    // megtartjuk a HMS LOG weboldal visszafelé kompatibilitásához.
    String json = "{";

    json += "\"systemState\":\"";
    switch (systemData.systemState)
    {
        case SYSTEM_OFF: json += "Üzemen kívül"; break;
        case SYSTEM_GAS: json += "Gáz"; break;
        case SYSTEM_WOOD: json += "Fa"; break;
        case SYSTEM_FIRE_WITHOUT_CIRCULATOR: json += "Tűz keringető nélkül"; break;
        case SYSTEM_STARTUP: json += "Indítás"; break;
        case SYSTEM_FAULT: json += "Hiba"; break;
        default: json += "Ismeretlen"; break;
    }
    json += "\",";

    json += "\"activeBranch\":\"";
    switch (systemData.activeBranch)
    {
        case HEATING_NONE: json += "Nincs"; break;
        case HEATING_GAS: json += "Gáz"; break;
        case HEATING_WOOD: json += "Fa"; break;
        case HEATING_ERROR: json += "Hiba"; break;
        default: json += "Ismeretlen"; break;
    }
    json += "\",";

    json += "\"alarmState\":\"";
    switch (systemData.alarmState)
    {
        case ALARM_NONE: json += "Nincs"; break;
        case ALARM_WARNING: json += "Figyelmeztetés"; break;
        case ALARM_CRITICAL: json += "Kritikus"; break;
        default: json += "Ismeretlen"; break;
    }
    json += "\",";

    json += "\"alarmReason\":\"";
    switch (systemData.alarmReason)
    {
        case ALARM_REASON_NONE: json += "Nincs"; break;
        case ALARM_REASON_BOTH_BRANCHES: json += "Gáz+fa ág egyszerre"; break;
        case ALARM_REASON_FIRE_WITHOUT_CIRCULATOR: json += "Tűz! Nincs keringető!"; break;
        case ALARM_REASON_WOOD_BOILER_OVERHEAT: json += "Kazán túlmelegedés"; break;
        case ALARM_REASON_STARTUP_FAULT: json += "Indítás sikertelen"; break;
        case ALARM_REASON_RTC_BATTERY: json += "RTC elem hiba"; break;
        case ALARM_REASON_RTC_OSCILLATOR_STOP: json += "RTC oszcillátor hiba"; break;
        case ALARM_REASON_SD_WARNING: json += "SD kártya eltávolítva"; break;
        case ALARM_REASON_SD_FULL: json += "SD kártya megtelt"; break;
        case ALARM_REASON_STARTUP_WARNING: json += "Indítás megszakadt"; break;
        case ALARM_REASON_WOOD_BOILER_WARNING: json += "Kazán hő magas"; break;
        case ALARM_REASON_GAS_FLOW_SENSOR: json += "Gáz előremenő szenzor"; break;
        case ALARM_REASON_GAS_RETURN_SENSOR: json += "Gáz visszat. szenzor"; break;
        case ALARM_REASON_WOOD_FLOW_SENSOR: json += "Fa előremenő szenzor"; break;
        case ALARM_REASON_WOOD_RETURN_SENSOR: json += "Fa visszatérő szenzor"; break;
        case ALARM_REASON_WOOD_BOILER_SENSOR: json += "Fa kazán szenzor hiba"; break;
        case ALARM_REASON_CHIMNEY_SENSOR: json += "Füstgáz szenzor hiba"; break;
        case ALARM_REASON_BOILER_ROOM_SENSOR: json += "Kazánház szenzor hiba"; break;
        default: json += "Ismeretlen"; break;
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

    json += "\"gasFlowTemperature\":" + String(systemData.gasFlowTemperature, 1) + ",";
    json += "\"gasReturnTemperature\":" + String(systemData.gasReturnTemperature, 1) + ",";
    json += "\"woodFlowTemperature\":" + String(systemData.woodFlowTemperature, 1) + ",";
    json += "\"woodReturnTemperature\":" + String(systemData.woodReturnTemperature, 1) + ",";
    json += "\"woodBoilerTemperature\":" + String(systemData.woodBoilerTemperature, 1) + ",";
    json += "\"chimneyTemperature\":" + String(systemData.chimneyTemperature, 1) + ",";
    json += "\"boilerRoomTemperature\":" + String(systemData.boilerRoomTemperature, 1) + ",";
    json += "\"boilerRoomHumidity\":" + String(systemData.boilerRoomHumidity, 1) + ",";

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
    json += "\"}";

    return json;
}

String buildApiStatusJson()
{
    String json = "{";
    json += "\"schemaVersion\":1,";
    appendTimestamp(json);
    json += ",";
    json += "\"uptimeSeconds\":";
    json += String(millis() / 1000UL);
    json += ",";

    unsigned long ageMs = 0;
    json += "\"dataAgeMs\":";
    if (getDataAgeMs(ageMs))
    {
        json += String(ageMs);
    }
    else
    {
        json += "null";
    }
    json += ",";

    json += "\"systemState\":\"";
    json += systemStateApiName(systemData.systemState);
    json += "\",";
    json += "\"activeBranch\":\"";
    json += heatingBranchApiName(systemData.activeBranch);
    json += "\",";
    json += "\"alarmState\":\"";
    json += alarmStateApiName(systemData.alarmState);
    json += "\",";
    json += "\"alarmReason\":\"";
    json += alarmReasonApiName(systemData.alarmReason);
    json += "\",";

    appendBoolean(json, "firePresent", systemData.firePresent);
    json += ",";
    appendBoolean(json, "gasAcPresent", systemData.gasAcPresent);
    json += ",";
    appendBoolean(json, "woodAcPresent", systemData.woodAcPresent);
    json += ",";

    json += "\"measurements\":{";
    appendMeasurement(json, "gasFlowTemperature", systemData.gasFlowTemperature, systemData.gasFlowSensorError);
    json += ",";
    appendMeasurement(json, "gasReturnTemperature", systemData.gasReturnTemperature, systemData.gasReturnSensorError);
    json += ",";
    appendMeasurement(json, "woodFlowTemperature", systemData.woodFlowTemperature, systemData.woodFlowSensorError);
    json += ",";
    appendMeasurement(json, "woodReturnTemperature", systemData.woodReturnTemperature, systemData.woodReturnSensorError);
    json += ",";
    appendMeasurement(json, "woodBoilerTemperature", systemData.woodBoilerTemperature, systemData.woodBoilerSensorError);
    json += ",";
    appendMeasurement(json, "chimneyTemperature", systemData.chimneyTemperature, systemData.chimneySensorError);
    json += ",";
    appendMeasurement(json, "boilerRoomTemperature", systemData.boilerRoomTemperature, systemData.boilerRoomSensorError);
    json += ",";
    // A páratartalomhoz a meglévő SystemData nem tartalmaz külön hibajelzőt.
    appendMeasurement(json, "boilerRoomHumidity", systemData.boilerRoomHumidity, systemData.boilerRoomSensorError);
    json += "},";

    json += "\"sensorErrors\":{";
    appendBoolean(json, "gasFlowSensorError", systemData.gasFlowSensorError);
    json += ",";
    appendBoolean(json, "gasReturnSensorError", systemData.gasReturnSensorError);
    json += ",";
    appendBoolean(json, "woodFlowSensorError", systemData.woodFlowSensorError);
    json += ",";
    appendBoolean(json, "woodReturnSensorError", systemData.woodReturnSensorError);
    json += ",";
    appendBoolean(json, "woodBoilerSensorError", systemData.woodBoilerSensorError);
    json += ",";
    appendBoolean(json, "chimneySensorError", systemData.chimneySensorError);
    json += ",";
    appendBoolean(json, "boilerRoomSensorError", systemData.boilerRoomSensorError);
    json += "},";

    json += "\"diagnostics\":{";
    appendBoolean(json, "wifiConnected", WiFi.status() == WL_CONNECTED);
    json += ",";
    appendBoolean(json, "sdCardAvailable", sdCardIsAvailable());
    json += ",";
    appendBoolean(json, "sdCardFull", sdCardIsFull());
    json += ",";
    appendBoolean(json, "rtcBatteryError", rtcBatteryWarning());
    json += ",";
    appendBoolean(json, "rtcOscillatorError", rtcOscillatorStopWarning());
    json += "}}";

    return json;
}

String buildApiHealthJson()
{
    String json = "{";
    json += "\"schemaVersion\":1,";
    json += "\"api\":\"ok\",";
    json += "\"uptimeSeconds\":";
    json += String(millis() / 1000UL);
    json += "}";
    return json;
}
