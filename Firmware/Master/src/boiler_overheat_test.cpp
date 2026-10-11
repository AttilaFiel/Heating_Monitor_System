#include <Arduino.h>

#include "boiler_overheat_test.h"
#include "system_data.h"
#include "config.h"

unsigned long boilerOverheatTestStartTime = 0;

void boilerOverheatTestSetup()
{
#if BOILER_OVERHEAT_TEST_ENABLED
    boilerOverheatTestStartTime = millis();
#endif
}

void boilerOverheatTestUpdate()
{
#if BOILER_OVERHEAT_TEST_ENABLED
    unsigned long elapsedTime =
        millis() - boilerOverheatTestStartTime;

    // 0–30 mp: normál mérés, nincs felülírás
    if (elapsedTime < BOILER_TEST_STEP_TIME)
        return;

    // 30–60 mp: 65 °C
    if (elapsedTime < BOILER_TEST_STEP_TIME * 2)
    {
        systemData.woodBoilerTemperature = 65.0;
        return;
    }

    // 60–90 mp: 75 °C
    if (elapsedTime < BOILER_TEST_STEP_TIME * 3)
    {
        systemData.woodBoilerTemperature = 75.0;
        return;
    }

    // 90–120 mp: 85 °C
    if (elapsedTime < BOILER_TEST_STEP_TIME * 4)
    {
        systemData.woodBoilerTemperature = 85.0;
        return;
    }

    // 120–150 mp: vissza 75 °C-ra
    if (elapsedTime < BOILER_TEST_STEP_TIME * 5)
    {
        systemData.woodBoilerTemperature = 75.0;
        return;
    }

    // 150–180 mp: ismét 85 °C
    if (elapsedTime < BOILER_TEST_STEP_TIME * 6)
    {
        systemData.woodBoilerTemperature = 85.0;
        return;
    }

    // 180–210 mp: vissza 65 °C-ra
    if (elapsedTime < BOILER_TEST_STEP_TIME * 7)
    {
        systemData.woodBoilerTemperature = 65.0;
        return;
    }

    // 210 mp után vége a tesztnek.
    // A normál szenzorfrissítés ismét érvényesül.
#endif
}