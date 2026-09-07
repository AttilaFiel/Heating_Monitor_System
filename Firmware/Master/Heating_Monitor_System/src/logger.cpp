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
#define MEASUREMENTS_LOG_INTERVAL 60000UL

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
            logMessage("LOGGER: HIBA - /log konyvtar nem hozhato letre!");
            return false;
        }
    }

    if (!SD.exists(yearPath))
    {
        if (!SD.mkdir(yearPath))
        {
            logMessage("LOGGER: HIBA - ev konyvtar nem hozhato letre!");
            return false;
        }
    }

    if (!SD.exists(monthPath))
    {
        if (!SD.mkdir(monthPath))
        {
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
            logMessage("LOGGER: HIBA - events.csv nem hozhato letre!");
            return false;
        }

        file.println(
            "timestamp,type,event,value_old,value_new"
        );

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
            logMessage(
                "LOGGER: HIBA - measurements.csv nem hozhato letre!"
            );

            return false;
        }

        file.println(
            "timestamp,gasFlow,gasReturn,woodFlow,woodReturn,"
            "woodBoiler,chimney,roomTemp,humidity,gasAC,woodAC"
        );

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
              const char* oldValue,
              const char* newValue)
{
    if (!loggerInitialized)
    {
        return false;
    }

    DateTime now = loggerNow();

    char timestamp[20];

    snprintf(timestamp,
             sizeof(timestamp),
             "%04d-%02d-%02d %02d:%02d:%02d",
             now.year(),
             now.month(),
             now.day(),
             now.hour(),
             now.minute(),
             now.second());

    // Kötelező mezők ellenőrzése még az SD megnyitása előtt
    if (timestamp[0] == '\0')
    {
        logMessage("LOGGER ERROR: timestamp ures!");
        return false;
    }

    if (type == nullptr || type[0] == '\0')
    {
        logMessage("LOGGER ERROR: type ures!");
        return false;
    }

    if (event == nullptr || event[0] == '\0')
    {
        logMessage("LOGGER ERROR: event ures!");
        return false;
    }

    char eventsPath[64];

    snprintf(eventsPath,
             sizeof(eventsPath),
             "/log/%04d/%02d/events.csv",
             now.year(),
             now.month());

    sdCardSetBusy(true);

    File file = SD.open(eventsPath, FILE_WRITE);

    if (!file)
    {
        sdCardSetBusy(false);

        logMessage("LOGGER: events.csv megnyitasi hiba!");
        loggerWriteError = true;
        loggerWriteErrorReason = LOGGER_WRITE_ERROR_EVENT_LOG;
        return false;
    }

    file.seek(file.size());

    bool writeOk = true;

    // Timestamp
    if (!writeLogField(file, "timestamp", timestamp, true))
    {
        writeOk = false;
    }

    // Vessző
    if (file.print(",") == 0)
    {
        writeOk = false;
    }

    // Type
    if (!writeLogField(file, "type", type, true))
    {
        writeOk = false;
    }

    // Vessző
    if (file.print(",") == 0)
    {
        writeOk = false;
    }

    // Event
    if (!writeLogField(file, "event", event, true))
    {
        writeOk = false;
    }

    // Vessző
    if (file.print(",") == 0)
    {
        writeOk = false;
    }

    // OldValue – opcionális
    if (!writeLogField(file, "oldValue", oldValue, false))
    {
        writeOk = false;
    }

    // Vessző
    if (file.print(",") == 0)
    {
        writeOk = false;
    }

    // NewValue – opcionális
    if (!writeLogField(file, "newValue", newValue, false))
    {
        writeOk = false;
    }

    // Sor lezárása
    if (file.println() == 0)
    {
        logMessage("LOGGER ERROR: sorvege iras HIBA");
        writeOk = false;
    }

    file.close();

    sdCardSetBusy(false);

    if (!writeOk)
    {
        logMessage("LOGGER: SD WRITE_ERROR - irasi hiba!");
        loggerWriteError = true;
        loggerWriteErrorReason = LOGGER_WRITE_ERROR_EVENT_LOG;
        return false;
    }

    return true;
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
        LOGGER_WRITE_ERROR_MEASUREMENT_LOG;

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

        return;
    }

    logMessage("LOGGER: SD kartya OK");

    // Könyvtárstruktúra
    if (!createLogDirectory())
    {
        return;
    }

    // CSV fájlok
    if (!createLogFiles())
    {
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