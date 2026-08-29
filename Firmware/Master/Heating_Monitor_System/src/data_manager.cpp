#include <Arduino.h>

#include "config.h"
#include "system_data.h"
#include "data_manager.h"
#include "network.h"


// --------------------------------
// Data Manager inicializalasa
// --------------------------------

void dataManagerSetup()
{
    logMessage("Data Manager inicializalva.");

    // Alapallapot

    systemData.gasFlowTemperature = 0.0;
    systemData.gasReturnTemperature = 0.0;

    systemData.woodFlowTemperature = 0.0;
    systemData.woodReturnTemperature = 0.0;

    systemData.woodBoilerTemperature = 0.0;
    systemData.chimneyTemperature = 0.0;

    systemData.boilerRoomTemperature = 0.0;
    systemData.boilerRoomHumidity = 0.0;

    systemData.gasAcPresent = false;
    systemData.woodAcPresent = false;

    systemData.firePresent = false;

    systemData.activeBranch = HEATING_NONE;

    systemData.systemState = SYSTEM_OFF;
}


// --------------------------------
// Adatok feldolgozasa
// --------------------------------

void dataManagerUpdate()
{
    // Egyelore meg nincs feldolgozasi logika.
    //
    // A Sensor Manager ide fogja adni
    // az aktualis meresi adatokat.
    //
    // Itt fogjuk kesobb meghatarozni:
    //
    // - melyik futesi ag aktiv
    // - van-e tuz
    // - van-e riasztasi allapot
    // - milyen egyeb feldolgozott allapotok vannak
}