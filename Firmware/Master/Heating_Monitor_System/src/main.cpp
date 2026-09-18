#include <Arduino.h>
#include <esp_system.h>

#include "network.h"
#include "i2c_scanner.h"
#include "rgb_led.h"
#include "rtc.h"
#include "sd_card.h"
#include "max6675.h"
#include "ds18b20.h"
#include "dht11.h"
#include "ac_sensor.h"
#include "display.h"
#include "buzzer.h"
#include "sensor_manager.h"
#include "data_manager.h"
#include "logger.h"
#include "esp32_system.h"

bool i2cScanned = false;
bool rtcTested = false;
bool sdCardTested = false;
bool max6675Tested = false;
bool ds18b20Tested = false;
bool dht11Tested = false;
bool acSensorTested = false;
bool displayTested = false;

unsigned long networkUpdateTime = 0;
#define NETWORK_UPDATE_INTERVAL 100UL

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

    // Logger indítása
    loggerSetup();

    // ESP32 rendszer indítása
    esp32SystemSetup();

    // Display indítása
    displaySetup();

    // Buzzer indítása
    buzzerSetup();
    
    // Data Manager indítása
    dataManagerSetup();

    // Sensor Manager indítása
    sensorManagerSetup();

    // RGB LED teszt
    Serial.println();
    Serial.println("====================");
    Serial.println("RGB LED TESZT");
    Serial.println("====================");


    // RGB rendszer indítása
    rgbLedSetup();


    // Induló színteszt
    startupTest();
}


void loop()
{
    // Wi-Fi, OTA és terminál kezelése
    if (millis() - networkUpdateTime >= NETWORK_UPDATE_INTERVAL)
    {
        networkUpdateTime = millis();

        networkLoop();
    }

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

        logMessage(
            systemData.gasAcPresent
                ? "GAS AC: VAN"
                : "GAS AC: NINCS"
        );

        logMessage(
            systemData.woodAcPresent
                ? "WOOD AC: VAN"
                : "WOOD AC: NINCS"
        );
        acSensorTested = true;
    }

    sensorManagerUpdate();
    dataManagerUpdate();
    
    sdCardUpdate();

    loggerUpdate();

    buzzerUpdate();
    rgbLedUpdate();

    // Véletlen RGB színek
    //randomLedShow();
    
    //dataManagerTest();

    // Display frissítése
    displayUpdate();
}