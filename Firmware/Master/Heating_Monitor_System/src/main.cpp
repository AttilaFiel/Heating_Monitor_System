#include <Arduino.h>

#include "network.h"
#include "i2c_scanner.h"
#include "rgb_led.h"
#include "rtc.h"
#include "sd_card.h"
#include "max6675.h"
#include "ds18b20.h"
#include "dht11.h"
#include "ac_sensor.h"

bool i2cScanned = false;
bool rtcTested = false;
bool sdCardTested = false;
bool max6675Tested = false;
bool ds18b20Tested = false;
bool dht11Tested = false;
bool acSensorTested = false;

void setup()
{
    Serial.begin(115200);

    Serial.println();
    Serial.println("ESP32 OTA + WIFI TERMINAL TESZT");


    // Hálózat indítása
    networkSetup();

    // I2C busz indítása
    i2cSetup();

    // RTC indítása
    rtcSetup();

    // MicroSD indítása
    sdCardSetup();

    // MAX6675 indítása
    max6675Setup();

    // DS18B20 indítása
    ds18b20Setup();

    // DHT11 indítása
    dht11Setup();

    // AC jelenlet erzekelok inditasa
    acSensorSetup();

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
    //waveShow();


    // Teljes káosz
    //chaosShow();


    Serial.println("VELETLEN SZINEK INDULNAK");
}


void loop()
{
    // Wi-Fi, OTA és terminál kezelése
    networkLoop();

    // I2C busz szkennelése
    // A keresés csak akkor indul,
    // amikor már csatlakozott a Wi-Fi terminál
    if (isTerminalConnected() && !i2cScanned)
    {
        scanI2C();

        i2cScanned = true;
    }

    if (isTerminalConnected() && !rtcTested)
    {
        rtcTest();

        rtcTested = true;
    }

    if (isTerminalConnected() && !sdCardTested)
    {
        sdCardTest();

        sdCardTested = true;
    }

    if (isTerminalConnected() && !max6675Tested)
    {
        max6675Test();

        max6675Tested = true;
    }

    if (isTerminalConnected() && !ds18b20Tested)
    {
        ds18b20Scan();

        ds18b20Tested = true;
    }

    if (isTerminalConnected() && !dht11Tested)
    {
        dht11Test();

        dht11Tested = true;
    }

    if (isTerminalConnected() && !acSensorTested)
    {
        acSensorTest();

        acSensorTested = true;
    }

    delay(5000);

    // Tesztüzenet
    //logMessage("ESP32 mukodik...");


    // Véletlen RGB színek
    randomLedShow();
}