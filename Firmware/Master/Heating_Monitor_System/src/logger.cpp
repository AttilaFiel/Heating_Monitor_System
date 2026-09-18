#include <Arduino.h>
#include <SD.h>

#include "config.h"
#include "network.h"
#include "rtc.h"
#include "logger.h"
#include "system_data.h"
#include "sd_card.h"


bool loggerInitialized = false;
bool loggerWriteError = false;

DateTime loggerNow()
{
    if (rtcIsAvailable())
    {
        return rtc.now();
    }

    return DateTime(2000, 1, 1, 0, 0, 0);
}

LoggerWriteErrorReason loggerWriteErrorReason =
    LOGGER_WRITE_ERROR_NONE;

unsigned long measurementsLogTime = 0;
#define MEASUREMENTS_LOG_INTERVAL 1000UL

// --------------------------------------------------
// Könyvtár létrehozása
// --------------------------------------------------

bool createLogDirectory()
{
    DateTime now = loggerNow();

    char yearPath[16];
    char monthPath[24];

    snprintf(
        yearPath,
        sizeof(yearPath),
        "/log/%04d",
        now.year()
    );

    snprintf(
        monthPath,
        sizeof(monthPath),
        "/log/%04d/%02d",
        now.year(),
        now.month()
    );

    if (!SD.exists("/log"))
    {
        if (!SD.mkdir("/log"))
        {
            sdCardWriteError("LOG_DIRECTORY");

            logMessage("LOGGER: HIBA - /log konyvtar nem hozhato letre!");
            return false;
        }
    }

    if (!SD.exists(yearPath))
    {
        if (!SD.mkdir(yearPath))
        {
            sdCardWriteError("LOG_DIRECTORY");

            logMessage("LOGGER: HIBA - ev konyvtar nem hozhato letre!");
            return false;
        }
    }

    if (!SD.exists(monthPath))
    {
        if (!SD.mkdir(monthPath))
        {
            sdCardWriteError("LOG_DIRECTORY");

            logMessage("LOGGER: HIBA - honap konyvtar nem hozhato letre!");
            return false;
        }
    }

    return true;
}


// --------------------------------------------------
// CSV fejléc létrehozása
// --------------------------------------------------

bool createLogFiles()
{
    DateTime now = loggerNow();

    char monthPath[24];

    snprintf(
        monthPath,
        sizeof(monthPath),
        "/log/%04d/%02d",
        now.year(),
        now.month()
    );

    char eventsPath[48];
    char measurementsPath[48];

    snprintf(
        eventsPath,
        sizeof(eventsPath),
        "%s/events.csv",
        monthPath
    );

    snprintf(
        measurementsPath,
        sizeof(measurementsPath),
        "%s/measurements.csv",
        monthPath
    );

    // ------------------------------
    // Eseménynapló
    // ------------------------------

    if (!SD.exists(eventsPath))
    {
        File file = SD.open(eventsPath, FILE_WRITE);

        if (!file)
        {
            sdCardWriteError("EVENT_LOG");

            logMessage("LOGGER: HIBA - events.csv nem hozhato letre!");
            return false;
        }

       if (file.println(
                "timestamp,type,event,value_old,value_new"
            ) == 0)
        {
            sdCardWriteError("EVENT_LOG");
            file.close();

            logMessage(
                "LOGGER: HIBA - events.csv fejléc irasi hiba!"
            );

            return false;
        }

        file.close();
    }

    // ------------------------------
    // Mérési napló
    // ------------------------------

    if (!SD.exists(measurementsPath))
    {
        File file = SD.open(measurementsPath, FILE_WRITE);

        if (!file)
        {
            sdCardWriteError("MEASUREMENT_LOG");

            logMessage(
                "LOGGER: HIBA - measurements.csv nem hozhato letre!"
            );

            return false;
        }

        if (file.println(
                "timestamp,gasFlow,gasReturn,woodFlow,woodReturn,"
                "woodBoiler,chimney,roomTemp,humidity,gasAC,woodAC"
            ) == 0)
        {
            sdCardWriteError("MEASUREMENT_LOG");
            file.close();

            logMessage(
                "LOGGER: HIBA - measurements.csv fejléc irasi hiba!"
            );

            return false;
        }

        file.close();
    }

    return true;
}

// --------------------------------------------------
// Esemény naplózása
// --------------------------------------------------
bool writeLogField(File& file,
                   const char* fieldName,
                   const char* value,
                   bool required)
{
    // Null pointer
    if (value == nullptr)
    {
        if (required)
        {
            logMessage("LOGGER ERROR: kotelezo mezo NULL!");
            logMessage(fieldName);
            return false;
        }

        return true;
    }

    // Üres érték
    if (value[0] == '\0')
    {
        if (required)
        {
            logMessage("LOGGER ERROR: kotelezo mezo ures!");
            logMessage(fieldName);
            return false;
        }

        // Opcionális mező üresen maradhat
        return true;
    }

    // Érték kiírása
    if (file.print(value) == 0)
    {
        logMessage("LOGGER ERROR: mezo iras HIBA");
        logMessage(fieldName);
        return false;
    }

    return true;
}

bool logEvent(const char* type,
              const char* event,
              const char* valueOld,
              const char* valueNew)
{
    DateTime now = loggerNow();

    if (!createLogDirectory())
    {
        logMessage("LOGGER ERROR: log konyvtar letrehozasi hiba!");

        loggerWriteError = true;
        loggerWriteErrorReason =
            LOGGER_WRITE_ERROR_EVENT_LOG;
            
        return false;
    }

    if (!createLogFiles())
    {
        logMessage("LOGGER ERROR: log fajlok letrehozasi hiba!");

        loggerWriteError = true;
        loggerWriteErrorReason =
            LOGGER_WRITE_ERROR_EVENT_LOG;

        return false;
    }

    char path[50];

    snprintf(
        path,
        sizeof(path),
        "/log/%04d/%02d/events.csv",
        now.year(),
        now.month()
    );

    sdCardSetBusy(true);

    File file = SD.open(path, FILE_APPEND);

    if (!file)
    {
        sdCardSetBusy(false);

        sdCardWriteError("EVENT_LOG");

        logMessage("LOGGER ERROR: events.csv megnyitasi hiba!");

        loggerWriteError = true;
        loggerWriteErrorReason =
            LOGGER_WRITE_ERROR_EVENT_LOG;

        return false;
    }

    bool success = true;

    if (file.size() == 0)
    {
        if (file.println("timestamp,type,event,value_old,value_new") == 0)
        {
            logMessage("LOGGER ERROR: events.csv fejlec irasi hiba!");
            success = false;
        }
    }

    if (success)
    {
        char timestamp[25];

        snprintf(
            timestamp,
            sizeof(timestamp),
            "%04d-%02d-%02d %02d:%02d:%02d",
            now.year(),
            now.month(),
            now.day(),
            now.hour(),
            now.minute(),
            now.second()
        );

        if (!writeLogField(file, "timestamp", timestamp, true))
            success = false;

        if (success && file.print(",") == 0)
            success = false;

        if (success && !writeLogField(file, "type", type, true))
            success = false;

        if (success && file.print(",") == 0)
            success = false;

        if (success && !writeLogField(file, "event", event, true))
            success = false;

        if (success && file.print(",") == 0)
            success = false;

        if (success && !writeLogField(file, "value_old", valueOld, false))
            success = false;

        if (success && file.print(",") == 0)
            success = false;

        if (success && !writeLogField(file, "value_new", valueNew, false))
            success = false;

        if (success && file.println() == 0)
            success = false;
    }

    file.close();

    sdCardSetBusy(false);

    if (!success)
    {
        sdCardWriteError("EVENT_LOG");

        logMessage("LOGGER ERROR: events.csv irasi hiba!");
        loggerWriteError = true;
        loggerWriteErrorReason =
            LOGGER_WRITE_ERROR_EVENT_LOG;
    }

    return success;
}

// --------------------------------------------------
// Mérési adatok naplózása
// --------------------------------------------------

bool logMeasurements()
{
    if (!loggerInitialized)
    {
        return false;
    }

    DateTime now = loggerNow();

    char monthPath[24];

    snprintf(
        monthPath,
        sizeof(monthPath),
        "/log/%04d/%02d",
        now.year(),
        now.month()
    );

    // Ha időközben új hónap kezdődött,
    // biztosítsuk a könyvtárstruktúra meglétét.
    if (!createLogDirectory())
    {
        loggerWriteError = true;
        loggerWriteErrorReason =
            LOGGER_WRITE_ERROR_MEASUREMENT_LOG;

        return false;
    }

    if (!createLogFiles())
    {
        loggerWriteError = true;
        loggerWriteErrorReason =
            LOGGER_WRITE_ERROR_MEASUREMENT_LOG;

        return false;
    }

    char measurementsPath[48];

    snprintf(
        measurementsPath,
        sizeof(measurementsPath),
        "%s/measurements.csv",
        monthPath
    );

    sdCardSetBusy(true);

    File file = SD.open(
        measurementsPath,
        FILE_APPEND
    );

    if (!file)
    {
        sdCardSetBusy(false);

        sdCardWriteError("MEASUREMENT_LOG");

        logMessage(
            "LOGGER: HIBA - measurements.csv nem nyithato meg!"
        );

        loggerWriteError = true;
        loggerWriteErrorReason =
            LOGGER_WRITE_ERROR_MEASUREMENT_LOG;

        return false;
    }

    char timestamp[24];

    snprintf(
        timestamp,
        sizeof(timestamp),
        "%04d.%02d.%02d %02d:%02d:%02d",
        now.year(),
        now.month(),
        now.day(),
        now.hour(),
        now.minute(),
        now.second()
    );

    bool writeOk = true;

    if (file.print(timestamp) == 0) writeOk = false;
    if (file.print(",") == 0) writeOk = false;

    if (file.print(systemData.gasFlowTemperature, 1) == 0) writeOk = false;
    if (file.print(",") == 0) writeOk = false;

    if (file.print(systemData.gasReturnTemperature, 1) == 0) writeOk = false;
    if (file.print(",") == 0) writeOk = false;

    if (file.print(systemData.woodFlowTemperature, 1) == 0) writeOk = false;
    if (file.print(",") == 0) writeOk = false;

    if (file.print(systemData.woodReturnTemperature, 1) == 0) writeOk = false;
    if (file.print(",") == 0) writeOk = false;

    if (file.print(systemData.woodBoilerTemperature, 1) == 0) writeOk = false;
    if (file.print(",") == 0) writeOk = false;

    if (file.print(systemData.chimneyTemperature, 1) == 0) writeOk = false;
    if (file.print(",") == 0) writeOk = false;

    if (file.print(systemData.boilerRoomTemperature, 1) == 0) writeOk = false;
    if (file.print(",") == 0) writeOk = false;

    if (file.print(systemData.boilerRoomHumidity, 1) == 0) writeOk = false;
    if (file.print(",") == 0) writeOk = false;

    if (file.print(systemData.gasAcPresent ? "ON" : "OFF") == 0)
        writeOk = false;

    if (file.print(",") == 0) writeOk = false;

    if (file.println(systemData.woodAcPresent ? "ON" : "OFF") == 0)
        writeOk = false;

    file.close();

    sdCardSetBusy(false);

    if (!writeOk)
    {
        sdCardWriteError("MEASUREMENT_LOG");

        logMessage(
            "LOGGER: SD WRITE_ERROR - meresi naplo irasi hiba!"
        );

        loggerWriteError = true;
        loggerWriteErrorReason =
            LOGGER_WRITE_ERROR_MEASUREMENT_LOG;

        return false;
    }

    loggerWriteError = false;
    loggerWriteErrorReason =
        LOGGER_WRITE_ERROR_NONE;

    return true;
}


// --------------------------------------------------
// Logger inicializálása
// --------------------------------------------------

void loggerSetup()
{
    logMessage("");
    logMessage("================================");
    logMessage("LOGGER INICIALIZALASA");
    logMessage("================================");

    DateTime now = loggerNow();

    // RTC ellenőrzése
    if (now.year() < 2000)
    {
        logMessage(
            "LOGGER: HIBA - ervenytelen RTC ido!"
        );

        return;
    }

    // SD inicializálása
    if (!SD.begin(MICROSD_CS, SPI, 400000))
    {
        logMessage(
            "LOGGER: HIBA - SD kartya nem inicializalhato!"
        );

        loggerWriteError = true;
        loggerWriteErrorReason =
            LOGGER_WRITE_ERROR_EVENT_LOG;

        return;
    }

    logMessage("LOGGER: SD kartya OK");

    // Könyvtárstruktúra
    if (!createLogDirectory())
    {
        loggerWriteError = true;
        loggerWriteErrorReason =
            LOGGER_WRITE_ERROR_EVENT_LOG;

        return;
    }

    // CSV fájlok
    if (!createLogFiles())
    {
        loggerWriteError = true;
        loggerWriteErrorReason =
            LOGGER_WRITE_ERROR_EVENT_LOG;

        return;
    }

    loggerInitialized = true;

    logMessage("LOGGER: INICIALIZALAS OK");

    logMessage("================================");
}

bool loggerHasWriteError()
{
    return loggerWriteError;
}

LoggerWriteErrorReason loggerGetWriteErrorReason()
{
    return loggerWriteErrorReason;
}

void loggerClearWriteError()
{
    loggerWriteError = false;
    loggerWriteErrorReason = LOGGER_WRITE_ERROR_NONE;
}

void loggerUpdate()
{
    if (millis() - measurementsLogTime <
        MEASUREMENTS_LOG_INTERVAL)
    {
        return;
    }

    measurementsLogTime = millis();

    logMeasurements();
}