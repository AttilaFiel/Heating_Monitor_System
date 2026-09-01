#include "config.h"
#include "system_data.h"
#include "data_manager.h"
#include "network.h"
#include "rtc.h"

unsigned long timestampLastUpdate = 0;
bool timestampInitialized = false;
bool firePresent;

bool startupActive = false;
unsigned long startupStartTime = 0;
unsigned long woodAcSwitchStartTime = 0;

bool woodAcSwitchTimerActive = false;
float startupInitialFlowTemperature = 0.0;

bool startupFault = false;
bool startupWarning = false;

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
    if (startupFault)
    {
        systemData.systemState = SYSTEM_FAULT;
        return;
    }

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

void updateStartup()
{
    // --------------------------------
    // Startup indítása
    // --------------------------------

    if (!startupActive && !startupFault)
    {
        if (systemData.gasAcPresent &&
            !systemData.woodAcPresent &&
            systemData.firePresent)
        {
            startupActive = true;

            startupStartTime = millis();

            woodAcSwitchTimerActive = false;
            
            startupWarning = false;

            systemData.systemState = SYSTEM_STARTUP;
        }

        return;
    }


    // --------------------------------
    // Sikeres átkapcsolás fára
    // --------------------------------

    if (systemData.woodAcPresent)
    {
        startupActive = false;
        woodAcSwitchTimerActive = false;
        startupFault = false;
        startupWarning = false;

        systemData.systemState = SYSTEM_WOOD;

        return;
    }


    // --------------------------------
    // Tűz megszűnt
    // --------------------------------

    if (!systemData.firePresent)
    {
        startupActive = false;
        woodAcSwitchTimerActive = false;
        startupFault = false;
        startupWarning = true;

        systemData.systemState = SYSTEM_GAS;

        return;
    }


    // --------------------------------
    // Wood flow elérte a 28 °C-ot
    // --------------------------------

    if (systemData.woodFlowTemperature >=
        WOOD_FLOW_HEATING_THRESHOLD)
    {
        if (!woodAcSwitchTimerActive)
        {
            woodAcSwitchTimerActive = true;
            woodAcSwitchStartTime = millis();
        }

        // 28 °C elérése után maximum 2 perc
        // áll rendelkezésre az AC átkapcsolására

        if (millis() - woodAcSwitchStartTime >=
            WOOD_AC_SWITCH_TIMEOUT)
        {
            startupActive = false;
            startupFault = true;
            systemData.systemState = SYSTEM_FAULT;

            return;
        }
    }


    // --------------------------------
    // 15 perces startup timeout
    // --------------------------------

    if (millis() - startupStartTime >= STARTUP_TIMEOUT)
    {
        startupActive = false;
        startupFault = true;
        systemData.systemState = SYSTEM_FAULT;

        return;
    }
}

void updateAlarmState()
{
    // Mindkét AC aktiv: rendszerhiba
    if (systemData.gasAcPresent &&
        systemData.woodAcPresent)
    {
        systemData.alarmState = ALARM_CRITICAL;
    }

    // Tűz van, de egyik fűtési ág sem aktív
    else if (!systemData.gasAcPresent &&
             !systemData.woodAcPresent &&
             systemData.firePresent)
    {
        systemData.alarmState = ALARM_CRITICAL;
    }

    // Fa kazán túlmelegedés
    else if (systemData.woodBoilerTemperature >=
             WOOD_BOILER_ALARM_TEMPERATURE)
    {
        systemData.alarmState = ALARM_CRITICAL;
    }

    // Startup hiba
    else if (startupFault)
    {
        systemData.alarmState = ALARM_CRITICAL;
    }

    else if (startupWarning)
    {
        systemData.alarmState = ALARM_WARNING;
    }

    // Fa kazán magas hőmérséklete
    else if (systemData.woodBoilerTemperature >=
             WOOD_BOILER_WARNING_TEMPERATURE)
    {
        systemData.alarmState = ALARM_WARNING;
    }

    // Minden rendben
    else
    {
        systemData.alarmState = ALARM_NONE;
    }
}


// --------------------------------
// Adatok frissitese
// --------------------------------
void dataManagerUpdate()
{
    updateFirePresent();
    updateActiveBranch();
    updateStartup();
    updateSystemState();
    updateAlarmState();
    updateTimestamp();
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
        "AC: gas=%d wood=%d | branch=%d | fire=%d | state=%d | alarm=%d",
        systemData.gasAcPresent,
        systemData.woodAcPresent,
        systemData.activeBranch,
        systemData.firePresent,
        systemData.systemState,
        systemData.alarmState
    );

    logMessage(message);
}