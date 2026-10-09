#include <Arduino.h>

#include "ac_presence_test.h"
#include "system_data.h"
#include "config.h"

enum AcPresenceTestStep
{
    AC_TEST_IDLE,
    AC_TEST_BOTH_ACTIVE,
    AC_TEST_FINISHED
};

AcPresenceTestStep acPresenceTestStep = AC_TEST_IDLE;
unsigned long acPresenceTestStartTime = 0;

void acPresenceTestSetup()
{
#if AC_PRESENCE_TEST_ENABLED
    acPresenceTestStartTime = millis();
    acPresenceTestStep = AC_TEST_IDLE;
#endif
}

void acPresenceTestUpdate()
{
#if AC_PRESENCE_TEST_ENABLED
    unsigned long elapsedTime =
        millis() - acPresenceTestStartTime;

    // 0–30 másodperc: mindkét ág kikapcsolva
    if (elapsedTime < AC_TEST_STEP_TIME)
    {
        acPresenceTestStep = AC_TEST_IDLE;

        systemData.gasAcPresent = false;
        systemData.woodAcPresent = false;
        return;
    }

    // 30–60 másodperc: mindkét ág aktív
    if (elapsedTime < AC_TEST_STEP_TIME * 2)
    {
        acPresenceTestStep = AC_TEST_BOTH_ACTIVE;

        systemData.gasAcPresent = true;
        systemData.woodAcPresent = true;
        return;
    }

    // 60 másodperctől: mindkét ág kikapcsolva
    acPresenceTestStep = AC_TEST_FINISHED;

    systemData.gasAcPresent = false;
    systemData.woodAcPresent = false;
#endif
}