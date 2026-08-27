#include <Arduino.h>
#include <DHT.h>

#include "config.h"
#include "network.h"
#include "dht11.h"


DHT dht(
    BOILER_ROOM_TEMPERATURE,
    DHT11
);


// DHT11 inicializálása
void dht11Setup()
{
    dht.begin();

    logMessage("DHT11 inicializalva.");
}


// DHT11 teszt
void dht11Test()
{
    logMessage("");
    logMessage("================================");
    logMessage("DHT11 TESZT INDUL");
    logMessage("================================");


    // A DHT11 lassú szenzor,
    // ezért hagyunk neki egy kis időt
    delay(2000);


    // Páratartalom kiolvasása
    float humidity = dht.readHumidity();


    // Hőmérséklet kiolvasása
    float temperature = dht.readTemperature();


    // Kommunikáció ellenőrzése
    if (isnan(humidity) || isnan(temperature))
    {
        logMessage("DHT11: HIBA - adat nem olvashato!");

        logMessage("================================");
        logMessage("DHT11 TESZT VEGE");
        logMessage("================================");

        return;
    }


    char message[100];


    snprintf(
        message,
        sizeof(message),
        "Kazanhelyiseg homerseklet: %.1f C",
        temperature
    );

    logMessage(message);


    snprintf(
        message,
        sizeof(message),
        "Paratartalom: %.1f %%",
        humidity
    );

    logMessage(message);


    logMessage("DHT11: OK");

    logMessage("================================");
    logMessage("DHT11 TESZT VEGE");
    logMessage("================================");
}