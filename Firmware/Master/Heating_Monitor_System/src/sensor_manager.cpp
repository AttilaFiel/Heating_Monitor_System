#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "config.h"
#include "sensor_manager.h"
#include "system_data.h"
#include "network.h"
#include "ds18b20.h"
#include "max6675.h"
#include "dht11.h"
#include "ac_sensor.h"

unsigned long dht11LastRead = 0;


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
// DS18B20 szenzorok ellenorzese
// --------------------------------
bool checkDS18B20Sensors()
{
    bool allOk = true;


    if (ds18b20.isConnected(gasFlowAddress))
    {
        logMessage("GAS_FLOW_SENSOR: OK");
    }
    else
    {
        logMessage("GAS_FLOW_SENSOR: HIBA - nem talalhato!");
        allOk = false;
    }


    if (ds18b20.isConnected(gasReturnAddress))
    {
        logMessage("GAS_RETURN_SENSOR: OK");
    }
    else
    {
        logMessage("GAS_RETURN_SENSOR: HIBA - nem talalhato!");
        allOk = false;
    }


    if (ds18b20.isConnected(woodFlowAddress))
    {
        logMessage("WOOD_FLOW_SENSOR: OK");
    }
    else
    {
        logMessage("WOOD_FLOW_SENSOR: HIBA - nem talalhato!");
        allOk = false;
    }


    if (ds18b20.isConnected(woodReturnAddress))
    {
        logMessage("WOOD_RETURN_SENSOR: OK");
    }
    else
    {
        logMessage("WOOD_RETURN_SENSOR: HIBA - nem talalhato!");
        allOk = false;
    }


    if (ds18b20.isConnected(woodBoilerAddress))
    {
        logMessage("WOOD_BOILER_SENSOR: OK");
    }
    else
    {
        logMessage("WOOD_BOILER_SENSOR: HIBA - nem talalhato!");
        allOk = false;
    }


    return allOk;
}

// --------------------------------
// Sensor Manager inicializalasa
// --------------------------------

void sensorManagerSetup()
{
    logMessage("Sensor Manager inicializalasa...");

    // MAX6675 indítása
    max6675Setup();

    // DS18B20 indítása
    ds18b20Setup();

    // DS18B20 szenzorok ellenőrzése
    if (checkDS18B20Sensors())
    {
        logMessage("DS18B20: minden szenzor OK");
    }
    else
    {
        logMessage("DS18B20: HIBA - legalabb egy szenzor hianyzik!");
    }

    // DHT11 indítása
    dht11Setup();

    // AC jelenlet erzekelok inditasa
    acSensorSetup();


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
// MAX6675 ertekek frissitese
// --------------------------------
void updateMAX6675()
{
    float temperature = max6675ReadTemperature();

    if (isnan(temperature))
    {
        logMessage("MAX6675: TERMOELEM HIBA / SZAKADAS!");

        return;
    }

    systemData.chimneyTemperature = temperature;
}

// --------------------------------
// AC jelenlet erzekelok frissitese
// --------------------------------

void updateACSensors()
{
    systemData.gasAcPresent = gasAcPresent();
    systemData.woodAcPresent = woodAcPresent();
}

// --------------------------------
// DHT11 ertekek frissitese
// --------------------------------
void updateDHT11()
{
    // DHT11 olvasasa 2 masodpercenkent
    if (millis() - dht11LastRead < 2000)
    {
        return;
    }

    dht11LastRead = millis();


    float temperature;
    float humidity;


    // Sikertelen meres
    if (!dht11Read(temperature, humidity))
    {
        return;
    }

    // Ervenyes adatok bekerulnek a kozponti adatstruktúrába
    systemData.boilerRoomTemperature = temperature;
    systemData.boilerRoomHumidity = humidity;
}

// --------------------------------
// Sensor Manager frissitese
// --------------------------------

void sensorManagerUpdate()
{
    // DS18B20
    updateDS18B20();

    // MAX6675
    updateMAX6675();

    // AC jelenlet erzekelok
    updateACSensors();

    // DHT11
    updateDHT11();
}