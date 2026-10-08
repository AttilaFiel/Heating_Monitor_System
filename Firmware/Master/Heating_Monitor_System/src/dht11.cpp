#include <Arduino.h>
#include <DHT.h>

#include "config.h"
#include "network.h"
#include "dht11.h"


// --------------------------------
// DHT11
// --------------------------------

DHT dht(
    BOILER_ROOM_TEMPERATURE,
    DHT11
);


// --------------------------------
// Inicializálás
// --------------------------------

void dht11Setup()
{
    dht.begin();

    logMessage("DHT11 inicializalva.");
}


// --------------------------------
// DHT11 teszt
// --------------------------------

void dht11Test()
{
    logMessage("");
    logMessage("================================");
    logMessage("DHT11 TESZT INDUL");
    logMessage("================================");

    // DHT11 inicializálása
    dht.begin();

    delay(2000);

    float temperature;
    float humidity;


    if (!dht11Read(temperature, humidity))
    {
        logMessage("DHT11: HIBA - nem olvashato adat!");

        logMessage("================================");
        logMessage("DHT11 TESZT VEGE");
        logMessage("================================");

        return;
    }


    char message[100];


    snprintf(
        message,
        sizeof(message),
        "Kazanhaz homerseklet: %.1f C",
        temperature
    );

    logMessage(message);


    snprintf(
        message,
        sizeof(message),
        "Kazanhaz paratartalom: %.1f %%",
        humidity
    );

    logMessage(message);


    logMessage("DHT11: OK");

    logMessage("================================");
    logMessage("DHT11 TESZT VEGE");
    logMessage("================================");
}

bool dht11Read(float& temperature, float& humidity)
{
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity))
    {
        return false;
    }

    return true;
}