#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "config.h"
#include "network.h"
#include "ds18b20.h"


OneWire oneWire(TEMPERATURE_SENSOR_BUS);

DallasTemperature ds18b20(&oneWire);


// Hexadecimális cím string átalakítása
// DS18B20 DeviceAddress formátumra
bool stringToAddress(const char* addressString, DeviceAddress address)
{
    if (strlen(addressString) != 16)
    {
        return false;
    }

    for (int i = 0; i < 8; i++)
    {
        char byteString[3];

        byteString[0] = addressString[i * 2];
        byteString[1] = addressString[i * 2 + 1];
        byteString[2] = '\0';

        address[i] = strtoul(byteString, nullptr, 16);
    }

    return true;
}


// Egyetlen DS18B20 kiolvasása
void readSensor(const char* sensorName, const char* sensorAddress)
{
    DeviceAddress address;

    char message[100];


    // Cím átalakítása
    if (!stringToAddress(sensorAddress, address))
    {
        snprintf(
            message,
            sizeof(message),
            "%s: HIBA - ervenytelen szenzor cim!",
            sensorName
        );

        logMessage(message);

        return;
    }


    // Ellenőrizzük, hogy a szenzor tényleg megtalálható-e
    if (!ds18b20.isConnected(address))
    {
        snprintf(
            message,
            sizeof(message),
            "%s: HIBA - szenzor nem talalhato!",
            sensorName
        );

        logMessage(message);

        return;
    }


    // Hőmérséklet kiolvasása
    float temperature = ds18b20.getTempC(address);


    // Hibás hőmérséklet ellenőrzése
    if (temperature == DEVICE_DISCONNECTED_C)
    {
        snprintf(
            message,
            sizeof(message),
            "%s: HIBA - homerseklet nem olvashato!",
            sensorName
        );

        logMessage(message);

        return;
    }


    snprintf(
        message,
        sizeof(message),
        "%s: %.2f C",
        sensorName,
        temperature
    );

    logMessage(message);
}


void ds18b20Setup()
{
    ds18b20.begin();

    logMessage("DS18B20 OneWire busz inicializalva.");
}


void ds18b20Scan()
{
    logMessage("");
    logMessage("================================");
    logMessage("DS18B20 SZENZOR TESZT INDUL");
    logMessage("================================");


    int deviceCount = ds18b20.getDeviceCount();

    char message[100];

    snprintf(
        message,
        sizeof(message),
        "Talalt eszkozok szama: %d",
        deviceCount
    );

    logMessage(message);


    if (deviceCount != 5)
    {
        snprintf(
            message,
            sizeof(message),
            "FIGYELEM: 5 szenzor vart, de %d talalva!",
            deviceCount
        );

        logMessage(message);
    }


    // Minden DS18B20 egyszerre megkezdi a mérést
    ds18b20.requestTemperatures();


    logMessage("--------------------");


    readSensor(
        "GAS_FLOW_SENSOR",
        GAS_FLOW_SENSOR
    );

    readSensor(
        "GAS_RETURN_SENSOR",
        GAS_RETURN_SENSOR
    );

    readSensor(
        "WOOD_FLOW_SENSOR",
        WOOD_FLOW_SENSOR
    );

    readSensor(
        "WOOD_RETURN_SENSOR",
        WOOD_RETURN_SENSOR
    );

    readSensor(
        "WOOD_BOILER_SENSOR",
        WOOD_BOILER_SENSOR
    );


    logMessage("================================");
    logMessage("DS18B20 SZENZOR TESZT VEGE");
    logMessage("================================");
}