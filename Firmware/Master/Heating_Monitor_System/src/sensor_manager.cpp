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
// Szenzorhiba számlálók
// --------------------------------
#define SENSOR_ERROR_THRESHOLD 3

uint8_t gasFlowSensorErrorCount = 0;
uint8_t gasReturnSensorErrorCount = 0;

uint8_t woodFlowSensorErrorCount = 0;
uint8_t woodReturnSensorErrorCount = 0;

uint8_t woodBoilerSensorErrorCount = 0;

uint8_t chimneySensorErrorCount = 0;

uint8_t boilerRoomSensorErrorCount = 0;

// --------------------------------
// DS18B20 egyedi címek
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
// DS18B20 szenzorok ellenőrzése
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
        logMessage("GAS_FLOW_SENSOR: HIBA - nem talalható!");
        allOk = false;
    }


    if (ds18b20.isConnected(gasReturnAddress))
    {
        logMessage("GAS_RETURN_SENSOR: OK");
    }
    else
    {
        logMessage("GAS_RETURN_SENSOR: HIBA - nem talalható!");
        allOk = false;
    }


    if (ds18b20.isConnected(woodFlowAddress))
    {
        logMessage("WOOD_FLOW_SENSOR: OK");
    }
    else
    {
        logMessage("WOOD_FLOW_SENSOR: HIBA - nem talalható!");
        allOk = false;
    }


    if (ds18b20.isConnected(woodReturnAddress))
    {
        logMessage("WOOD_RETURN_SENSOR: OK");
    }
    else
    {
        logMessage("WOOD_RETURN_SENSOR: HIBA - nem talalható!");
        allOk = false;
    }


    if (ds18b20.isConnected(woodBoilerAddress))
    {
        logMessage("WOOD_BOILER_SENSOR: OK");
    }
    else
    {
        logMessage("WOOD_BOILER_SENSOR: HIBA - nem talalható!");
        allOk = false;
    }


    return allOk;
}

// --------------------------------
// Sensor Manager inicializálása
// --------------------------------

void sensorManagerSetup()
{
    logMessage("Sensor Manager inicializálása...");

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
        logMessage("DS18B20: HIBA - legalább egy szenzor hiányzik!");
    }

    // DHT11 indítása
    dht11Setup();

    // AC jelenlét érzékelők indítása
    acSensorSetup();


}

// --------------------------------
// DS18B20 értékek frissítése
// --------------------------------
void updateDS18B20()
{
    // Mérés indítása az összes szenzoron
    ds18b20.requestTemperatures();

    float temperature;

    // Gáz előremenő
    temperature =
        readSensorTemperature(GAS_FLOW_SENSOR);

    if (!isnan(temperature))
    {
        // Sikeres mérés
        gasFlowSensorErrorCount = 0;
        systemData.gasFlowSensorError = false;

        // Utolsó érvényes érték frissítése
        systemData.gasFlowTemperature = temperature;
    }
    else
    {
        // Hibás mérés
        if (gasFlowSensorErrorCount < SENSOR_ERROR_THRESHOLD)
        {
            gasFlowSensorErrorCount++;
        }

        if (gasFlowSensorErrorCount >= SENSOR_ERROR_THRESHOLD)
        {
            systemData.gasFlowSensorError = true;
        }
    }

    // Gáz visszatérő
    temperature =
        readSensorTemperature(GAS_RETURN_SENSOR);

    if (!isnan(temperature))
    {
        // Sikeres mérés
        gasReturnSensorErrorCount = 0;
        systemData.gasReturnSensorError = false;

        // Utolsó érvényes érték frissítése
        systemData.gasReturnTemperature = temperature;
    }
    else
    {
        // Hibás mérés
        if (gasReturnSensorErrorCount < SENSOR_ERROR_THRESHOLD)
        {
            gasReturnSensorErrorCount++;
        }

        if (gasReturnSensorErrorCount >= SENSOR_ERROR_THRESHOLD)
        {
            systemData.gasReturnSensorError = true;
        }
    }


    // Fatüzelésű kazán előremenő
    temperature =
        readSensorTemperature(WOOD_FLOW_SENSOR);

    if (!isnan(temperature))
    {
        // Sikeres mérés
        woodFlowSensorErrorCount = 0;
        systemData.woodFlowSensorError = false;

        // Utolsó érvényes érték frissítése
        systemData.woodFlowTemperature = temperature;
    }
    else
    {
        // Hibás mérés
        if (woodFlowSensorErrorCount < SENSOR_ERROR_THRESHOLD)
        {
            woodFlowSensorErrorCount++;
        }

        if (woodFlowSensorErrorCount >= SENSOR_ERROR_THRESHOLD)
        {
            systemData.woodFlowSensorError = true;
        }
    }

    // Fatüzelésű kazán visszatérő
    temperature =
        readSensorTemperature(WOOD_RETURN_SENSOR);

    if (!isnan(temperature))
    {
        // Sikeres mérés
        woodReturnSensorErrorCount = 0;
        systemData.woodReturnSensorError = false;

        // Utolsó érvényes érték frissítése
        systemData.woodReturnTemperature = temperature;
    }
    else
    {
        // Hibás mérés
        if (woodReturnSensorErrorCount < SENSOR_ERROR_THRESHOLD)
        {
            woodReturnSensorErrorCount++;
        }

        if (woodReturnSensorErrorCount >= SENSOR_ERROR_THRESHOLD)
        {
            systemData.woodReturnSensorError = true;
        }
    }

    // Fatüzelésű kazán
    temperature =
        readSensorTemperature(WOOD_BOILER_SENSOR);

    if (!isnan(temperature))
    {
        // Sikeres mérés
        woodBoilerSensorErrorCount = 0;
        systemData.woodBoilerSensorError = false;

        // Utolsó érvényes érték frissítése
        systemData.woodBoilerTemperature = temperature;
    }
    else
    {
        // Hibás mérés
        if (woodBoilerSensorErrorCount < SENSOR_ERROR_THRESHOLD)
        {
            woodBoilerSensorErrorCount++;
        }

        if (woodBoilerSensorErrorCount >= SENSOR_ERROR_THRESHOLD)
        {
            systemData.woodBoilerSensorError = true;
        }
    }

    updateSensorReadStats();
}

// --------------------------------
// MAX6675 értékek frissítése
// --------------------------------
void updateMAX6675()
{
    float temperature = max6675ReadTemperature();

    if (!isnan(temperature))
    {
        chimneySensorErrorCount = 0;
        systemData.chimneySensorError = false;
        systemData.chimneyTemperature = temperature;
    }
    else
    {
        if (chimneySensorErrorCount < SENSOR_ERROR_THRESHOLD)
        {
            chimneySensorErrorCount++;
        }

        if (chimneySensorErrorCount >= SENSOR_ERROR_THRESHOLD)
        {
            systemData.chimneySensorError = true;
        }

        logMessage("MAX6675: TERMOELEM HIBA / SZAKADÁS!");
    }
}

// --------------------------------
// AC jelenlét érzékelők frissítése
// --------------------------------

void updateACSensors()
{
    systemData.gasAcPresent = gasAcPresent();
    systemData.woodAcPresent = woodAcPresent();
}

// --------------------------------
// DHT11 értékek frissítése
// --------------------------------
void updateDHT11()
{
    // DHT11 olvasása 2 másodpercenként
    if (millis() - dht11LastRead < 2000)
    {
        return;
    }

    dht11LastRead = millis();

    float temperature;
    float humidity;

    // Sikertelen mérés
    if (!dht11Read(temperature, humidity))
    {
        if (boilerRoomSensorErrorCount < SENSOR_ERROR_THRESHOLD)
        {
            boilerRoomSensorErrorCount++;
        }

        if (boilerRoomSensorErrorCount >= SENSOR_ERROR_THRESHOLD)
        {
            systemData.boilerRoomSensorError = true;
        }

        logMessage("DHT11: OLVASáSI HIBA!");

        return;
    }

    // Sikeres mérés
    boilerRoomSensorErrorCount = 0;
    systemData.boilerRoomSensorError = false;

    // Érvényes adatok bekerülnek a központi adatstruktúrába
    systemData.boilerRoomTemperature = temperature;
    systemData.boilerRoomHumidity = humidity;
}

// --------------------------------
// Sensor Manager frissítése
// --------------------------------
void sensorManagerUpdate()
{
    // DS18B20
    updateDS18B20();

    // MAX6675
    updateMAX6675();

    // AC jelenlét érzékelők
    updateACSensors();

    // DHT11
    updateDHT11();
}