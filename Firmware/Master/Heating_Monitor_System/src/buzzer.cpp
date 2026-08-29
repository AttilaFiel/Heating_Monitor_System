#include <Arduino.h>
#include "config.h"
#include "network.h"

void buzzerTest()
{
    logMessage("Buzzer teszt");

    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, LOW);

    digitalWrite(BUZZER, HIGH);
    delay(500);
    digitalWrite(BUZZER, LOW);
}