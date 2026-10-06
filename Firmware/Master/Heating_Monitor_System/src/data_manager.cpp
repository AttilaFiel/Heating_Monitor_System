#include "config.h"
#include "system_data.h"
#include "data_manager.h"
#include "network.h"
#include "rtc.h"
#include "logger.h"
#include "sd_card.h"

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

// Logger globalis változók
SystemState previousSystemState = SYSTEM_OFF;
bool systemStateInitialized = false;
HeatingBranch previousActiveBranch = HEATING_NONE;
bool activeBranchInitialized = false;
AlarmState previousAlarmState = ALARM_NONE;
bool alarmStateInitialized = false;
bool previousFirePresent = false;
bool firePresentInitialized = false;
bool previousGasAcPresent = false;
bool gasAcPresentInitialized = false;
bool previousWoodAcPresent = false;
bool woodAcPresentInitialized = false;

bool startupFlowThresholdAbove = false;
enum OverheatState
{
    OVERHEAT_NORMAL,
    OVERHEAT_WARNING,
    OVERHEAT_CRITICAL
};

OverheatState previousOverheatState = OVERHEAT_NORMAL;
bool overheatStateInitialized = false;

bool isStartupActive()
{
    return startupActive;
}

// AC-jelenlétérzékelők türelmi ideje
#define AC_SWITCH_GRACE_TIME 1000

unsigned long acBothActiveStartTime = 0;

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
// Időbélyeg frissítése
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
    // Rendszerindítás
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
            startupFlowThresholdAbove = false;

            logEvent(
                "STARTUP",
                "START",
                "",
                ""
            );

            systemData.systemState = SYSTEM_STARTUP;
        }

        return;
    }


    // --------------------------------
    // Sikeres átkapcsolás fára
    // --------------------------------

    if (systemData.woodAcPresent)
    {
        if (woodAcSwitchTimerActive)
        {
            logEvent(
                "STARTUP",
                "SWITCH_TIMER_STOP",
                "",
                "SUCCESS"
            );
        }

        logEvent(
            "STARTUP",
            "SUCCESS",
            "",
            ""
        );

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
        if (woodAcSwitchTimerActive)
        {
            logEvent(
                "STARTUP",
                "SWITCH_TIMER_STOP",
                "",
                "FIRE_LOST"
            );
        }

        logEvent(
            "STARTUP",
            "FIRE_LOST",
            "",
            ""
        );
        startupActive = false;
        woodAcSwitchTimerActive = false;
        startupFault = false;
        startupWarning = true;
        startupFlowThresholdAbove = false;

        systemData.systemState = SYSTEM_GAS;

        return;
    }


    // --------------------------------
    // A fatüzelésű kazán előremenő hőmérséklete elérte a 28 °C-ot
    // --------------------------------
    if (systemData.woodFlowTemperature >= WOOD_FLOW_HEATING_THRESHOLD &&
    !startupFlowThresholdAbove)
    {
        startupFlowThresholdAbove = true;

        logEvent(
            "STARTUP",
            "FLOW_THRESHOLD",
            "BELOW",
            "ABOVE"
        );
    }
    else if (systemData.woodFlowTemperature < WOOD_FLOW_HEATING_THRESHOLD &&
            startupFlowThresholdAbove)
    {
        startupFlowThresholdAbove = false;

        logEvent(
            "STARTUP",
            "FLOW_THRESHOLD",
            "ABOVE",
            "BELOW"
        );
    }

    if (systemData.woodFlowTemperature >= WOOD_FLOW_HEATING_THRESHOLD)
    {
        if (!woodAcSwitchTimerActive)
        {
            woodAcSwitchTimerActive = true;
            woodAcSwitchStartTime = millis();

            logEvent(
                "STARTUP",
                "SWITCH_TIMER_START",
                "",
                ""
            );
        }

        // 28 °C elérése után maximum 2 perc
        // áll rendelkezésre az AC átkapcsolására

        if (millis() - woodAcSwitchStartTime >= WOOD_AC_SWITCH_TIMEOUT)
        {
            logEvent(
                "STARTUP",
                "SWITCH_TIMER_TIMEOUT",
                "",
                ""
            );

            woodAcSwitchTimerActive = false;
            startupActive = false;
            startupFault = true;
            systemData.systemState = SYSTEM_FAULT;

            return;
        }
    }


    // --------------------------------
    // 15 perces indítási időkorlát
    // --------------------------------

    if (millis() - startupStartTime >= STARTUP_TIMEOUT)
    {
        logEvent(
            "STARTUP",
            "TIMEOUT",
            "",
            ""
        );

        startupActive = false;
        startupFault = true;
        systemData.systemState = SYSTEM_FAULT;
        return;
    }
}

void updateAlarmState()
{
    // Mindkét AC aktiv: átkapcsoláskor 1 másodperc türelmi idő
    if (systemData.gasAcPresent &&
        systemData.woodAcPresent)
    {
        if (acBothActiveStartTime == 0)
        {
            acBothActiveStartTime = millis();
        }

        if (millis() - acBothActiveStartTime >= AC_SWITCH_GRACE_TIME)
        {
            systemData.alarmState = ALARM_CRITICAL;
            systemData.alarmReason = ALARM_REASON_BOTH_BRANCHES;
        }
        else
        {
            systemData.alarmState = ALARM_NONE;
            systemData.alarmReason = ALARM_REASON_NONE;
        }

        return;
    }

    // Nincs két aktív ág, ezért az átkapcsolási időzítő törölhető
    acBothActiveStartTime = 0;

    // Tűz van, de egyik fűtési ág sem aktív
    if (!systemData.gasAcPresent &&
        !systemData.woodAcPresent &&
        systemData.firePresent)
    {
        systemData.alarmState = ALARM_CRITICAL;
        systemData.alarmReason = ALARM_REASON_FIRE_WITHOUT_CIRCULATOR;
    }

    // Fa kazán túlmelegedés
    else if (systemData.woodBoilerTemperature >=
             WOOD_BOILER_ALARM_TEMPERATURE)
    {
        systemData.alarmState = ALARM_CRITICAL;
        systemData.alarmReason = ALARM_REASON_WOOD_BOILER_OVERHEAT;
    }

    // Indítási hiba
    else if (startupFault)
    {
        systemData.alarmState = ALARM_CRITICAL;
        systemData.alarmReason = ALARM_REASON_STARTUP_FAULT;
    }

    // RTC elem hiba
    else if (rtcBatteryWarning())
    {
        systemData.alarmState = ALARM_WARNING;
        systemData.alarmReason = ALARM_REASON_RTC_BATTERY;
    }

    // RTC oszcillátor leállás hiba
    else if (rtcOscillatorStopWarning())
    {
        systemData.alarmState = ALARM_WARNING;
        systemData.alarmReason = ALARM_REASON_RTC_OSCILLATOR_STOP;
    }

    // SD kártya hiba
    else if (sdCardHasWarning())
    {
        systemData.alarmState = ALARM_WARNING;
        systemData.alarmReason = ALARM_REASON_SD_WARNING;
    }

    // SD kártya megtelt
    else if (sdCardIsFull())
    {
        systemData.alarmState = ALARM_WARNING;
        systemData.alarmReason = ALARM_REASON_SD_FULL;
    }

    // Indítási figyelmeztetés
    else if (startupWarning)
    {
        systemData.alarmState = ALARM_WARNING;
        systemData.alarmReason = ALARM_REASON_STARTUP_WARNING;
    }

    // Fa kazán magas hőmérséklete
    else if (systemData.woodBoilerTemperature >=
             WOOD_BOILER_WARNING_TEMPERATURE)
    {
        systemData.alarmState = ALARM_WARNING;
        systemData.alarmReason = ALARM_REASON_WOOD_BOILER_WARNING;
    }

    // Minden rendben
    else
    {
        systemData.alarmState = ALARM_NONE;
        systemData.alarmReason = ALARM_REASON_NONE;
    }
}


// --------------------------------
// Rendszerállapot naplózási szövege
// --------------------------------

const char* systemStateToLogString(SystemState state)
{
    switch (state)
    {
        case SYSTEM_OFF:
            return "OFF";

        case SYSTEM_GAS:
            return "GAS";

        case SYSTEM_WOOD:
            return "WOOD";

        case SYSTEM_FIRE_WITHOUT_CIRCULATOR:
            return "FIRE_WITHOUT_CIRCULATOR";

        case SYSTEM_STARTUP:
            return "STARTUP";

        case SYSTEM_FAULT:
            return "FAULT";

        default:
            return "UNKNOWN";
    }
}

// --------------------------------
// Fűtési ág állapotának naplózási szövege
// --------------------------------

const char* heatingBranchToLogString(HeatingBranch branch)
{
    switch (branch)
    {
        case HEATING_NONE:
            return "NONE";

        case HEATING_GAS:
            return "GAS";

        case HEATING_WOOD:
            return "WOOD";

        case HEATING_ERROR:
            return "ERROR";

        default:
            return "UNKNOWN";
    }
}

// --------------------------------
// Riasztási állapot naplózási szövege
// --------------------------------
const char* alarmStateToLogString(AlarmState state)
{
    switch (state)
    {
        case ALARM_NONE:
            return "NONE";

        case ALARM_WARNING:
            return "WARNING";

        case ALARM_CRITICAL:
            return "CRITICAL";

        default:
            return "UNKNOWN";
    }
}

// --------------------------------
// Rendszerállapot logolása
// --------------------------------
void updateSystemStateLog()
{
    if (!systemStateInitialized)
    {
        previousSystemState = systemData.systemState;
        systemStateInitialized = true;
        return;
    }

    if (systemData.systemState != previousSystemState)
    {
        logEvent(
            "SYSTEM",
            "STATE",
            systemStateToLogString(previousSystemState),
            systemStateToLogString(systemData.systemState)
        );

        previousSystemState = systemData.systemState;
    }
}

// --------------------------------
// Fűtési ág állapot logolása
// --------------------------------
void updateActiveBranchLog()
{
    if (!activeBranchInitialized)
    {
        previousActiveBranch = systemData.activeBranch;
        activeBranchInitialized = true;
        return;
    }

    if (systemData.activeBranch != previousActiveBranch)
    {
        logEvent(
            "BRANCH",
            "STATE",
            heatingBranchToLogString(previousActiveBranch),
            heatingBranchToLogString(systemData.activeBranch)
        );

        previousActiveBranch = systemData.activeBranch;
    }
}

// --------------------------------
// Riasztási állapot logolása
// --------------------------------
void updateAlarmStateLog()
{
    if (!alarmStateInitialized)
    {
        previousAlarmState = systemData.alarmState;
        alarmStateInitialized = true;
        return;
    }

    if (systemData.alarmState != previousAlarmState)
    {
        logEvent(
            "ALARM",
            "STATE",
            alarmStateToLogString(previousAlarmState),
            alarmStateToLogString(systemData.alarmState)
        );

        previousAlarmState = systemData.alarmState;
    }
}

// --------------------------------
// Tűz jelenlét logolása
// --------------------------------
void updateFirePresentLog()
{
    if (!firePresentInitialized)
    {
        previousFirePresent = systemData.firePresent;
        firePresentInitialized = true;
        return;
    }

    if (systemData.firePresent != previousFirePresent)
    {
        logEvent(
            "FIRE",
            "STATE",
            previousFirePresent ? "ON" : "OFF",
            systemData.firePresent ? "ON" : "OFF"
        );

        previousFirePresent = systemData.firePresent;
    }
}

// --------------------------------
// Gáz AC jelenlét logolása
// --------------------------------
void updateGasAcPresentLog()
{
    if (!gasAcPresentInitialized)
    {
        previousGasAcPresent = systemData.gasAcPresent;
        gasAcPresentInitialized = true;
        return;
    }

    if (systemData.gasAcPresent != previousGasAcPresent)
    {
        logEvent(
            "GAS_AC",
            "STATE",
            previousGasAcPresent ? "ON" : "OFF",
            systemData.gasAcPresent ? "ON" : "OFF"
        );

        previousGasAcPresent = systemData.gasAcPresent;
    }
}

// --------------------------------
// Fa AC jelenlét logolása
// --------------------------------
void updateWoodAcPresentLog()
{
    if (!woodAcPresentInitialized)
    {
        previousWoodAcPresent = systemData.woodAcPresent;
        woodAcPresentInitialized = true;
        return;
    }

    if (systemData.woodAcPresent != previousWoodAcPresent)
    {
        logEvent(
            "WOOD_AC",
            "STATE",
            previousWoodAcPresent ? "ON" : "OFF",
            systemData.woodAcPresent ? "ON" : "OFF"
        );

        previousWoodAcPresent = systemData.woodAcPresent;
    }
}

// --------------------------------
// Fa kazán túlmelegedés megállapítása
// --------------------------------
OverheatState getOverheatState()
{
    if (systemData.woodBoilerTemperature >=
        WOOD_BOILER_ALARM_TEMPERATURE)
    {
        return OVERHEAT_CRITICAL;
    }

    if (systemData.woodBoilerTemperature >=
        WOOD_BOILER_WARNING_TEMPERATURE)
    {
        return OVERHEAT_WARNING;
    }

    return OVERHEAT_NORMAL;
}

// --------------------------------
// A fakazán túlmelegedési állapotának szöveges megjelenítése
// --------------------------------
const char* overheatStateToLogString(OverheatState state)
{
    switch (state)
    {
        case OVERHEAT_NORMAL:
            return "NORMAL";

        case OVERHEAT_WARNING:
            return "WARNING";

        case OVERHEAT_CRITICAL:
            return "CRITICAL";

        default:
            return "UNKNOWN";
    }
}

// --------------------------------
// Fa kazán túlmelegedés állapot logolása
// --------------------------------
void updateOverheatStateLog()
{
    OverheatState currentState = getOverheatState();

    if (!overheatStateInitialized)
    {
        previousOverheatState = currentState;
        overheatStateInitialized = true;
        return;
    }

    if (currentState != previousOverheatState)
    {
        logEvent(
            "OVERHEAT",
            "STATE",
            overheatStateToLogString(previousOverheatState),
            overheatStateToLogString(currentState)
        );

        previousOverheatState = currentState;
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

    rtcUpdate();
    updateTimestamp();

    updateSystemStateLog();
    updateActiveBranchLog();
    updateAlarmStateLog();
    updateFirePresentLog();
    updateGasAcPresentLog();
    updateWoodAcPresentLog();
    updateOverheatStateLog();
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