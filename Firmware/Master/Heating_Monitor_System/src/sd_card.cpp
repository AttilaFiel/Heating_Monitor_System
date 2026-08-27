#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "config.h"
#include "network.h"
#include "sd_card.h"


// SPI inicializálása
void sdCardSetup()
{
    // Minden SPI eszkoz kezdetben inaktiv

    pinMode(MICROSD_CS, OUTPUT);
    digitalWrite(MICROSD_CS, HIGH);

    pinMode(CHIMNEY_TEMPERATURE_CS, OUTPUT);
    digitalWrite(CHIMNEY_TEMPERATURE_CS, HIGH);
    
    // SPI busz inicializálása
    SPI.begin(
        SPI_SCK,
        SPI_MISO,
        SPI_MOSI,
        MICROSD_CS
    );

    logMessage("SPI busz inicializalva.");
}


// MicroSD teljes teszt
void sdCardTest()
{
    logMessage("");
    logMessage("================================");
    logMessage("MICROSD TESZT INDUL");
    logMessage("================================");
    
    // MAX6675 biztosan nincs kivalasztva
    digitalWrite(CHIMNEY_TEMPERATURE_CS, HIGH);

    // SD kártya inicializálása
    logMessage("SD kartya inicializalasa...");

    if (!SD.begin(MICROSD_CS, SPI, 400000))
    {
        logMessage("MICROSD: HIBA - kartya nem inicializalhato!");
        return;
    }

    logMessage("MICROSD: OK");


    // Kártya típusa
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE)
    {
        logMessage("MICROSD: HIBA - nincs kartya!");
        return;
    }

    switch (cardType)
    {
        case CARD_MMC:
            logMessage("Kartya tipus: MMC");
            break;

        case CARD_SD:
            logMessage("Kartya tipus: SDSC");
            break;

        case CARD_SDHC:
            logMessage("Kartya tipus: SDHC");
            break;

        default:
            logMessage("Kartya tipus: ISMERETLEN");
            break;
    }


    // Kapacitás
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);

    char message[100];

    snprintf(
        message,
        sizeof(message),
        "Kartya kapacitas: %llu MB",
        (unsigned long long)cardSize
    );

    logMessage(message);


    // Tesztfájl neve
    const char* testFileName = "/sd_test.txt";


    // Régi tesztfájl törlése
    if (SD.exists(testFileName))
    {
        SD.remove(testFileName);

        logMessage("Regi tesztfajl torolve.");
    }


    // Tesztadat
    const char* testData = "Heating Monitor System - MicroSD teszt";


    // Fájl létrehozása és írás
    logMessage("Tesztfajl letrehozasa es irasa...");

    File file = SD.open(testFileName, FILE_WRITE);

    if (!file)
    {
        logMessage("HIBA: Tesztfajl nem nyithato meg irasra!");
        return;
    }

    file.print(testData);

    file.close();

    logMessage("Adat irasa: OK");


    // Fájl visszaolvasása
    logMessage("Tesztfajl visszaolvasasa...");

    file = SD.open(testFileName, FILE_READ);

    if (!file)
    {
        logMessage("HIBA: Tesztfajl nem nyithato meg olvasasra!");
        return;
    }


    String readData = "";

    while (file.available())
    {
        readData += (char)file.read();
    }

    file.close();


    // Kiolvasott adat
    logMessage("Kiolvasott adat:");

    logMessage(readData.c_str());


    // Összehasonlítás
    if (readData == testData)
    {
        logMessage("IRASI / OLVASASI TESZT: OK");
    }
    else
    {
        logMessage("IRASI / OLVASASI TESZT: HIBA!");
    }


    logMessage("================================");
    logMessage("MICROSD TESZT VEGE");
    logMessage("================================");
}