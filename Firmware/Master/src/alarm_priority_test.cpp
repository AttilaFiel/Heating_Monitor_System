#include <Arduino.h>

#include "alarm_priority_test.h"
#include "system_data.h"
#include "config.h"

// =========================================================
// RIASZTÁSI PRIORITÁS TESZT
// =========================================================
//
// Időzítés:
//   0 s   → indulás
//  30 s   → alacsony prioritású riasztás
//  60 s   → magasabb prioritású riasztás
//  90 s   → magasabb prioritás megszűnik
// 120 s   → alacsonyabb prioritás megszűnik
//
// A második teszthez elegendő ezt a sorrendet módosítani.
// =========================================================


// ---------------------------------------------------------
// Tesztállapotok
// ---------------------------------------------------------
enum AlarmTestStep
{
    ALARM_TEST_IDLE,

    ALARM_TEST_LOW_ACTIVE,
    ALARM_TEST_HIGH_ACTIVE,

    ALARM_TEST_HIGH_RELEASED,
    ALARM_TEST_FINISHED
};


AlarmTestStep alarmTestStep = ALARM_TEST_IDLE;

unsigned long alarmTestStartTime = 0;


// ---------------------------------------------------------
// Teszt inicializálása
// ---------------------------------------------------------
void alarmPriorityTestSetup()
{
#if ALARM_PRIORITY_TEST_ENABLED

    alarmTestStartTime = millis();

    alarmTestStep = ALARM_TEST_IDLE;

#endif
}


// ---------------------------------------------------------
// Teszt frissítése
// ---------------------------------------------------------
void alarmPriorityTestUpdate()
{
#if ALARM_PRIORITY_TEST_ENABLED

    unsigned long elapsedTime =
        millis() - alarmTestStartTime;


    // =====================================================
    // 0–30 s
    // Normál állapot
    // =====================================================
    if (elapsedTime < ALARM_TEST_STEP_TIME)
    {
        alarmTestStep = ALARM_TEST_IDLE;

        return;
    }


    // =====================================================
    // 30–60 s
    // Alacsonyabb prioritású riasztás
    //
    // Kazánház / DHT11 szenzorhiba
    // WARNING
    // =====================================================
    if (elapsedTime < ALARM_TEST_STEP_TIME * 2)
    {
        alarmTestStep = ALARM_TEST_LOW_ACTIVE;

        systemData.boilerRoomSensorError = true;

        return;
    }


    // =====================================================
    // 60–90 s
    // Alacsonyabb + magasabb prioritású riasztás
    //
    // Kazánház szenzorhiba  → WARNING
    // Gáz előremenő hiba    → CRITICAL
    //
    // A magasabb prioritásúnak kell érvényesülnie.
    // =====================================================
    if (elapsedTime < ALARM_TEST_STEP_TIME * 3)
    {
        alarmTestStep = ALARM_TEST_HIGH_ACTIVE;

        systemData.boilerRoomSensorError = true;
        systemData.gasFlowSensorError = true;

        return;
    }


    // =====================================================
    // 90–120 s
    // A magasabb prioritású hiba megszűnik
    //
    // Kazánház szenzorhiba továbbra is fennáll.
    //
    // Elvárt:
    // WARNING / BOILER_ROOM_SENSOR
    // =====================================================
    if (elapsedTime < ALARM_TEST_STEP_TIME * 4)
    {
        alarmTestStep = ALARM_TEST_HIGH_RELEASED;

        systemData.boilerRoomSensorError = true;
        systemData.gasFlowSensorError = false;

        return;
    }


    // =====================================================
    // 120 s után
    // Minden teszthiba megszűnik
    // =====================================================
    alarmTestStep = ALARM_TEST_FINISHED;

    systemData.boilerRoomSensorError = false;
    systemData.gasFlowSensorError = false;

#endif
}