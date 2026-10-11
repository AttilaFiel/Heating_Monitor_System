#include <Arduino.h>

#include "config.h"
#include "network.h"
#include "system_data.h"
#include "buzzer.h"


// Buzzer időzítések
#define BUZZER_WARNING_INTERVAL   2000UL
#define BUZZER_WARNING_DURATION   10UL

#define BUZZER_CRITICAL_INTERVAL  500UL
#define BUZZER_CRITICAL_DURATION  500UL


unsigned long buzzerStartTime = 0;
bool buzzerActive = false;


void buzzerSetup()
{
    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, LOW);

    buzzerStartTime = millis();
    buzzerActive = false;
}


void buzzerUpdate()
{
    unsigned long now = millis();

    if (systemData.alarmState == ALARM_NONE)
    {
        digitalWrite(BUZZER, LOW);
        buzzerActive = false;
        return;
    }


    unsigned long interval;
    unsigned long duration;


    if (systemData.alarmState == ALARM_WARNING)
    {
        interval = BUZZER_WARNING_INTERVAL;
        duration = BUZZER_WARNING_DURATION;
    }
    else
    {
        interval = BUZZER_CRITICAL_INTERVAL;
        duration = BUZZER_CRITICAL_DURATION;
    }


    if (!buzzerActive)
    {
        if (now - buzzerStartTime >= interval)
        {
            digitalWrite(BUZZER, HIGH);

            buzzerActive = true;
            buzzerStartTime = now;
        }
    }
    else
    {
        if (now - buzzerStartTime >= duration)
        {
            digitalWrite(BUZZER, LOW);

            buzzerActive = false;
            buzzerStartTime = now;
        }
    }
}


void buzzerTest()
{
    logMessage("Buzzer teszt");

    digitalWrite(BUZZER, LOW);

    digitalWrite(BUZZER, HIGH);
    delay(500);
    digitalWrite(BUZZER, LOW);
}