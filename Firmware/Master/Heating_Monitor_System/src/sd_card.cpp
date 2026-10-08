#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "config.h"
#include "network.h"
#include "sd_card.h"
#include "logger.h"

bool sdCardAvailable = false;
bool sdCardStateInitialized = false;

bool sdWriteErrorPending = false;
LoggerWriteErrorReason sdWriteErrorReason = LOGGER_WRITE_ERROR_NONE;

bool sdCardBusy = false;

bool sdCardIsAvailable()
{
    return sdCardAvailable;
}

enum SDWarningReason
{
    SD_WARNING_NONE,
    SD_WARNING_REMOVED,
    SD_WARNING_INIT_ERROR,
    SD_WARNING_WRITE_ERROR,
    SD_WARNING_FULL
};

unsigned long sdCardCheckTime = 0;

#define SD_CARD_CHECK_INTERVAL 2000

#define SD_FULL_THRESHOLD_PERCENT 95
SDWarningReason sdWarning = SD_WARNING_NONE;

bool sdCardIsFull()
{
    return sdWarning == SD_WARNING_FULL;
}

bool sdCardHasWarning()
{
    return sdWarning != SD_WARNING_NONE;
}

bool sdCardIsBusy()
{
    return sdCardBusy;
}

void sdCardSetBusy(bool busy)
{
    sdCardBusy = busy;
}

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
    
    // MAX6675 biztosan nincs kiválasztva
    digitalWrite(CHIMNEY_TEMPERATURE_CS, HIGH);

    // SD kártya inicializálása
    logMessage("SD kártya inicializalasa...");

    if (!SD.begin(MICROSD_CS, SPI, 400000))
    {
        logMessage("MICROSD: HIBA - kártya nem inicializálható!");
        return;
    }

    logMessage("MICROSD: OK");


    // Kártya típusa
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE)
    {
        logMessage("MICROSD: HIBA - nincs kártya!");
        return;
    }

    switch (cardType)
    {
        case CARD_MMC:
            logMessage("Kártya típus: MMC");
            break;

        case CARD_SD:
            logMessage("Kártya típus: SDSC");
            break;

        case CARD_SDHC:
            logMessage("Kártya típus: SDHC");
            break;

        default:
            logMessage("Kártya típus: ISMERETLEN");
            break;
    }


    // Kapacitás
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);

    char message[100];

    snprintf(
        message,
        sizeof(message),
        "Kártya kapacitas: %llu MB",
        (unsigned long long)cardSize
    );

    logMessage(message);


    // Tesztfájl neve
    const char* testFileName = "/sd_test.txt";


    // Régi tesztfájl törlése
    if (SD.exists(testFileName))
    {
        SD.remove(testFileName);

        logMessage("Regi tesztfájl törölve.");
    }


    // Tesztadat
    const char* testData = "Heating Monitor System - MicroSD teszt";


    // Fájl létrehozása és írás
    logMessage("Tesztfájl létrehozása es írása...");

    File file = SD.open(testFileName, FILE_WRITE);

    if (!file)
    {
        logMessage("HIBA: Tesztfájl nem nyitható meg írásra!");
        return;
    }

    file.print(testData);

    file.close();

    logMessage("Adat írása: OK");


    // Fájl visszaolvasása
    logMessage("Tesztfájl visszaolvasása...");

    file = SD.open(testFileName, FILE_READ);

    if (!file)
    {
        logMessage("HIBA: Tesztfájl nem nyitható meg olvasasra!");
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
        logMessage("ÍRÁSI / OLVASASI TESZT: OK");
    }
    else
    {
        logMessage("ÍRÁSI / OLVASASI TESZT: HIBA!");
    }


    logMessage("================================");
    logMessage("MICROSD TESZT VÉGE");
    logMessage("================================");
}

void sdCardUpdate()
{
    if (millis() - sdCardCheckTime < SD_CARD_CHECK_INTERVAL)
    {
        return;
    }

    sdCardCheckTime = millis();

    //bool available = sdCardAvailable;
    File testFile = SD.open("/log");
    bool available = testFile;
    testFile.close();
    /* SD.end();

    bool available = SD.begin(
        MICROSD_CS,
        SPI,
        400000
    ); */

    // Logger írási hiba átvétele
    if (loggerHasWriteError())
    {
        sdWriteErrorPending = true;
        sdWriteErrorReason = loggerGetWriteErrorReason();
    }

    // Első állapotfelmérés
    if (!sdCardStateInitialized)
    {
        sdCardStateInitialized = true;
        sdCardAvailable = available;

        if (available)
        {
            logEvent(
                "SD",
                "MOUNT_OK",
                "",
                ""
            );
        }
        else
        {
            logEvent(
                "SD",
                "MOUNT_ERROR",
                "",
                "NOT_FOUND"
            );
        }

        return;
    }

    // SD eltűnt
    if (!available)
    {
        if (sdCardAvailable)
        {
            sdCardAvailable = false;
            sdWarning = SD_WARNING_REMOVED;

            logEvent(
                "SD",
                "MOUNT_ERROR",
                "",
                "REMOVED"
            );
        }

        return;
    }

    // SD visszatért
    if (!sdCardAvailable)
    {
        bool success = logEvent(
            "SD",
            "RECOVERED",
            "",
            ""
        );

        if (success)
        {
            sdCardAvailable = true;
            sdWarning = SD_WARNING_NONE;
        }

        return;
    }

    // SD telítettség ellenőrzése
    uint64_t totalBytes = SD.totalBytes();
    uint64_t usedBytes = SD.usedBytes();

    if (totalBytes > 0)
    {
        uint64_t fullThreshold =
            (totalBytes * SD_FULL_THRESHOLD_PERCENT) / 100;

        if (usedBytes >= fullThreshold)
        {
            if (sdWarning != SD_WARNING_FULL)
            {
                sdWarning = SD_WARNING_FULL;

                logEvent(
                    "SD",
                    "FULL",
                    "",
                    ""
                );
            }
        }
        else
        {
            if (sdWarning == SD_WARNING_FULL)
            {
                sdWarning = SD_WARNING_NONE;

                logEvent(
                    "SD",
                    "RECOVERED",
                    "",
                    ""
                );
            }
        }
    }


    // Függőben lévő írási hiba naplózása
    if (sdWriteErrorPending)
    {
        const char* reason = "";

        if (sdWriteErrorReason ==
            LOGGER_WRITE_ERROR_EVENT_LOG)
        {
            reason = "EVENT_LOG";
        }
        else if (sdWriteErrorReason ==
                 LOGGER_WRITE_ERROR_MEASUREMENT_LOG)
        {
            reason = "MEASUREMENT_LOG";
        }

        bool success = logEvent(
            "SD",
            "WRITE_ERROR",
            "",
            reason
        );

        if (success)
        {
            sdWriteErrorPending = false;
            sdWriteErrorReason = LOGGER_WRITE_ERROR_NONE;

            loggerClearWriteError();
        }
    }
}

void sdCardWriteError(const char* reason)
{
    logMessage("SD HIBA: írási hiba!");

    if (sdWarning != SD_WARNING_WRITE_ERROR)
    {
        sdWarning = SD_WARNING_WRITE_ERROR;
    }
}