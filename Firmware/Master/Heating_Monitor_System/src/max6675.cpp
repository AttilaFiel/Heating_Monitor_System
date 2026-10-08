#include <Arduino.h>
#include <SPI.h>

#include "config.h"
#include "network.h"
#include "max6675.h"


void max6675Setup()
{
    // MAX6675 alapállapotban nincs kiválasztva
    pinMode(CHIMNEY_TEMPERATURE_CS, OUTPUT);
    digitalWrite(CHIMNEY_TEMPERATURE_CS, HIGH);

    logMessage("MAX6675 elokeszitve.");
}

float max6675ReadTemperature()
{
    // SD kártya biztosan nincs kiválasztva
    digitalWrite(MICROSD_CS, HIGH);

    // MAX6675 kiválasztása
    digitalWrite(CHIMNEY_TEMPERATURE_CS, LOW);

    SPI.beginTransaction(
        SPISettings(
            4000000,
            MSBFIRST,
            SPI_MODE0
        )
    );

    // 16 bit kiolvasása
    uint16_t rawData = SPI.transfer16(0x0000);

    SPI.endTransaction();

    // MAX6675 kikapcsolása
    digitalWrite(CHIMNEY_TEMPERATURE_CS, HIGH);


    // Termoelem hiba / szakadás
    if (rawData & 0x0004)
    {
        return NAN;
    }


    // Hőmérséklet bitek
    rawData >>= 3;

    // Egy bit = 0.25 Celsius
    float temperature = rawData * 0.25;

    return temperature;
}

void max6675Test()
{
    logMessage("");
    logMessage("================================");
    logMessage("MAX6675 TESZT INDUL");
    logMessage("================================");


    // SD kártya biztosan nincs kiválasztva
    digitalWrite(MICROSD_CS, HIGH);


    // MAX6675 kiválasztása
    digitalWrite(CHIMNEY_TEMPERATURE_CS, LOW);


    SPI.beginTransaction(
        SPISettings(
            4000000,
            MSBFIRST,
            SPI_MODE0
        )
    );


    // 16 bit kiolvasása
    uint16_t rawData = SPI.transfer16(0x0000);


    SPI.endTransaction();


    // MAX6675 kikapcsolása a buszról
    digitalWrite(CHIMNEY_TEMPERATURE_CS, HIGH);


    char message[100];


    // Nyers adat kiírása
    snprintf(
        message,
        sizeof(message),
        "MAX6675 nyers adat: 0x%04X",
        rawData
    );

    logMessage(message);


    // D2 bit: termoelem hiba
    if (rawData & 0x0004)
    {
        logMessage("MAX6675: TERMOELEM HIBA / SZAKADAS!");

        logMessage("================================");
        logMessage("MAX6675 TESZT VEGE");
        logMessage("================================");

        return;
    }


    // Az alsó 3 bit nem hőmérséklet adat
    rawData >>= 3;


    // Egy bit = 0.25 Celsius
    float temperature = rawData * 0.25;


    snprintf(
        message,
        sizeof(message),
        "Kemeny homerseklet: %.2f C",
        temperature
    );

    logMessage(message);

    logMessage("MAX6675: OK");

    logMessage("================================");
    logMessage("MAX6675 TESZT VEGE");
    logMessage("================================");
}