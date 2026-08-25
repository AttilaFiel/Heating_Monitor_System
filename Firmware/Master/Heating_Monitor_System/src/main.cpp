#include <Arduino.h>

#include "network.h"
#include "rgb_led.h"


void setup()
{
    Serial.begin(115200);

    Serial.println();
    Serial.println("ESP32 OTA + WIFI TERMINAL TESZT");


    // Hálózat indítása
    networkSetup();


    // RGB LED teszt
    Serial.println();
    Serial.println("====================");
    Serial.println("RGB LED TESZT");
    Serial.println("====================");


    // RGB rendszer indítása
    rgbLedSetup();


    // Induló színteszt
    startupTest();


    // Hullámzó fényjáték
    waveShow();


    // Teljes káosz
    chaosShow();


    Serial.println("VELETLEN SZINEK INDULNAK");
}


void loop()
{
    // Wi-Fi, OTA és terminál kezelése
    networkLoop();


    // Tesztüzenet
    logMessage("ESP32 mukodik...");


    // Véletlen RGB színek
    randomLedShow();
}