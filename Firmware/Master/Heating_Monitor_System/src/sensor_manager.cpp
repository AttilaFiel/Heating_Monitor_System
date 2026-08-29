#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "config.h"
#include "sensor_manager.h"
#include "system_data.h"
#include "network.h"
#include "ds18b20.h"

// --------------------------------
// Sensor Manager uzenet buffer
// --------------------------------

char message[100];

// --------------------------------
// DS18B20 egyedi cimek
// --------------------------------

DeviceAddress gasFlowAddress  =
{
    0x28, 0x50, 0xD6, 0x89, 0x90, 0x25, 0x06, 0x73
};

DeviceAddress gasReturnAddress  =
{
    0x28, 0x42, 0xFF, 0x95, 0x90, 0x25, 0x06, 0x67
};

DeviceAddress woodFlowAddress =
{
    0x28, 0xCE, 0x77, 0x39, 0x90, 0x25, 0x06, 0xBE
};

DeviceAddress woodReturnAddress =
{
    0x28, 0x71, 0x1D, 0x9E, 0x91, 0x25, 0x06, 0xB5
};

DeviceAddress woodBoilerAddress =
{
    0x28, 0x27, 0xC4, 0x2D, 0x91, 0x25, 0x06, 0x9D
};


// --------------------------------
// Sensor Manager inicializalasa
// --------------------------------

void sensorManagerSetup()
{
    logMessage("Sensor Manager inicializalasa...");

    ds18b20.begin();

    uint8_t sensorCount = ds18b20.getDeviceCount();

    snprintf(
        message,
        sizeof(message),
        "DS18B20 szenzorok szama: %u",
        sensorCount
    );

    logMessage(message);


    if (sensorCount != 5)
    {
        logMessage("FIGYELEM: nem 5 DS18B20 talalhato!");
    }
    else
    {
        logMessage("DS18B20: 5 szenzor OK");
    }
}


// --------------------------------
// DS18B20 ertekek frissitese
// --------------------------------

void updateDS18B20()
{
    // Meres inditasa az osszes szenzoron

    ds18b20.requestTemperatures();


    // GAS FLOW

    systemData.gasFlowTemperature =
        ds18b20.getTempC(gasFlowAddress);


    // GAS RETURN

    systemData.gasReturnTemperature =
        ds18b20.getTempC(gasReturnAddress);


    // WOOD FLOW

    systemData.woodFlowTemperature =
        ds18b20.getTempC(woodFlowAddress);


    // WOOD RETURN

    systemData.woodReturnTemperature =
        ds18b20.getTempC(woodReturnAddress);


    // WOOD BOILER

    systemData.woodBoilerTemperature =
        ds18b20.getTempC(woodBoilerAddress);
}


// --------------------------------
// Sensor Manager frissitese
// --------------------------------

void sensorManagerUpdate()
{
    updateDS18B20();


    // --------------------------------
    // TESZT KIIRAS
    // --------------------------------

    snprintf(
        message,
        sizeof(message),
        "GAS FLOW: %.2f C",
        systemData.gasFlowTemperature
    );

    logMessage(message);


    snprintf(
        message,
        sizeof(message),
        "GAS RETURN: %.2f C",
        systemData.gasReturnTemperature
    );

    logMessage(message);


    snprintf(
        message,
        sizeof(message),
        "WOOD FLOW: %.2f C",
        systemData.woodFlowTemperature
    );

    logMessage(message);


    snprintf(
        message,
        sizeof(message),
        "WOOD RETURN: %.2f C",
        systemData.woodReturnTemperature
    );

    logMessage(message);


    snprintf(
        message,
        sizeof(message),
        "WOOD BOILER: %.2f C",
        systemData.woodBoilerTemperature
    );

    logMessage(message);
}