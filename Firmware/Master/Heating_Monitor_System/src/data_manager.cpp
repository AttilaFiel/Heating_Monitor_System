#include "system_data.h"
#include "data_manager.h"
#include "network.h"
#include "rtc.h"

unsigned long timestampLastUpdate = 0;
bool timestampInitialized = false;
bool firePresent;
#define FIRE_TEMPERATURE_THRESHOLD 80.0


// --------------------------------
// Data Manager inicializalasa
// --------------------------------

void dataManagerSetup()
{
    logMessage("Data Manager inicializalva.");

    // Alapallapot

    systemData.gasFlowTemperature = 0.0;
    systemData.gasReturnTemperature = 0.0;

    systemData.woodFlowTemperature = 0.0;
    systemData.woodReturnTemperature = 0.0;

    systemData.woodBoilerTemperature = 0.0;
    systemData.chimneyTemperature = 0.0;

    systemData.boilerRoomTemperature = 0.0;
    systemData.boilerRoomHumidity = 0.0;

    systemData.gasAcPresent = false;
    systemData.woodAcPresent = false;

    systemData.firePresent = false;

    systemData.activeBranch = HEATING_NONE;

    systemData.systemState = SYSTEM_OFF;
}

// --------------------------------
// Timestamp frissitese
// --------------------------------
void updateTimestamp()
{
    if (!timestampInitialized ||
        millis() - timestampLastUpdate >= 1000)
    {
        timestampLastUpdate = millis();
        timestampInitialized = true;

        systemData.timestamp = rtc.now();
    }

    timestampLastUpdate = millis();

    systemData.timestamp = rtc.now();
}

// --------------------------------
// Adatok feldolgozasa
// --------------------------------
void updateActiveBranch()
{
    if (systemData.gasAcPresent && systemData.woodAcPresent)
    {
        systemData.activeBranch = HEATING_ERROR;
    }
    else if (systemData.gasAcPresent)
    {
        systemData.activeBranch = HEATING_GAS;
    }
    else if (systemData.woodAcPresent)
    {
        systemData.activeBranch = HEATING_WOOD;
    }
    else
    {
        systemData.activeBranch = HEATING_NONE;
    }
}

void updateFirePresent()
{
    systemData.firePresent =
        systemData.chimneyTemperature >= FIRE_TEMPERATURE_THRESHOLD;
}

void updateSystemState()
{
    switch (systemData.activeBranch)
    {
        case HEATING_NONE:

            if (systemData.firePresent)
            {
                systemData.systemState =
                    SYSTEM_FIRE_WITHOUT_CIRCULATOR;
            }
            else
            {
                systemData.systemState =
                    SYSTEM_OFF;
            }

            break;


        case HEATING_GAS:

            if (systemData.firePresent)
            {
                systemData.systemState =
                    SYSTEM_STARTUP;
            }
            else
            {
                systemData.systemState =
                    SYSTEM_GAS;
            }

            break;


        case HEATING_WOOD:

            systemData.systemState =
                SYSTEM_WOOD;

            break;


        case HEATING_ERROR:

            systemData.systemState =
                SYSTEM_FAULT;

            break;
    }
}

// --------------------------------
// Adatok frissitese
// --------------------------------
void dataManagerUpdate()
{
    updateTimestamp();
    updateActiveBranch();
    updateFirePresent();
    updateSystemState();
    // Egyelore meg nincs feldolgozasi logika.
    //
    // A Sensor Manager ide fogja adni
    // az aktualis meresi adatokat.
    //
    // Itt fogjuk kesobb meghatarozni:
    //
    // - melyik futesi ag aktiv
    // - van-e tuz
    // - van-e riasztasi allapot
    // - milyen egyeb feldolgozott allapotok vannak
}

// --------------------------------
// Data Manager teszt
// --------------------------------
void dataManagerTest()
{
    char message[100];

    snprintf(
        message,
        sizeof(message),
        "AC: gas=%d wood=%d | branch=%d | fire=%d | state=%d",
        systemData.gasAcPresent,
        systemData.woodAcPresent,
        systemData.activeBranch,
        systemData.firePresent,
        systemData.systemState
    );

    logMessage(message);
}