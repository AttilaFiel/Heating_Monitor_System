#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>

#include "network.h"
#include "rtc.h"
#include "logger.h"

RTC_DS3231 rtc;
enum RTCWarningReason
{
    RTC_WARNING_NONE,
    RTC_WARNING_LOST_POWER,
    RTC_WARNING_NOT_FOUND,
    RTC_WARNING_READ_ERROR,
    RTC_WARNING_OSCILLATOR_STOP,
    RTC_WARNING_INVALID_TIME
};

RTCWarningReason rtcWarning = RTC_WARNING_NONE;

bool rtcAvailable = false;

bool rtcBatteryWarning()
{
    return rtcWarning == RTC_WARNING_LOST_POWER;
}

bool rtcOscillatorStopWarning()
{
    return rtcWarning == RTC_WARNING_OSCILLATOR_STOP;
}

// RTC inicializálása
void rtcSetup()
{
    rtcAvailable = false;

    if (!rtc.begin())
    {
        logMessage("DS3231 RTC HIBA: nem inicializalhato!");
        return;
    }

    rtcAvailable = true;

    logMessage("DS3231 RTC inicializalva.");

    if (rtc.lostPower())
    {
        logMessage("RTC elvesztette az idot - ido alaphelyzetbe allitasa.");

        rtcWarning = RTC_WARNING_LOST_POWER;

        logEvent(
            "RTC",
            "ERROR",
            "",
            "LOST_POWER"
        );

        rtc.adjust(DateTime(2000, 1, 1, 0, 0, 0));

        logMessage("RTC ido: 2000.01.01 00:00:00");
    }
}

bool rtcIsAvailable()
{
    return rtcAvailable;
}

// RTC állapotának folyamatos ellenőrzése
void rtcUpdate()
{
    // DS3231 I2C ellenőrzése
    Wire.beginTransmission(0x68);

    if (Wire.endTransmission() != 0)
    {
        if (rtcWarning != RTC_WARNING_NOT_FOUND)
        {
            logEvent(
                "RTC",
                "ERROR",
                "",
                "NOT_FOUND"
            );

            rtcWarning = RTC_WARNING_NOT_FOUND;
        }

        return;
    }


    // RTC idő kiolvasása
    Wire.beginTransmission(0x68);

    Wire.write(0x00);

    if (Wire.endTransmission(false) != 0)
    {
        if (rtcWarning != RTC_WARNING_READ_ERROR)
        {
            logEvent(
                "RTC",
                "ERROR",
                "",
                "READ_ERROR"
            );

            rtcWarning = RTC_WARNING_READ_ERROR;
        }

        return;
    }

    if (Wire.requestFrom(0x68, 7) != 7)
    {
        if (rtcWarning != RTC_WARNING_READ_ERROR)
        {
            logEvent(
                "RTC",
                "ERROR",
                "",
                "READ_ERROR"
            );

            rtcWarning = RTC_WARNING_READ_ERROR;
        }

        return;
    }

    // A regiszterek kiolvasása
    while (Wire.available())
    {
        Wire.read();
    }


    DateTime now = rtc.now();


    // RTC oszcillátor leállt
    if (rtc.lostPower())
    {
        if (rtcWarning != RTC_WARNING_OSCILLATOR_STOP)
        {
            logEvent(
                "RTC",
                "ERROR",
                "",
                "OSCILLATOR_STOP"
            );

            rtcWarning = RTC_WARNING_OSCILLATOR_STOP;
        }

        return;
    }


    // Érvénytelen idő
    DateTime minimumTime(2000, 1, 1, 0, 0, 0);

    if (now < minimumTime)
    {
        if (rtcWarning != RTC_WARNING_INVALID_TIME)
        {
            logEvent(
                "RTC",
                "ERROR",
                "",
                "INVALID_TIME"
            );

            rtcWarning = RTC_WARNING_INVALID_TIME;
        }

        return;
    }


    // RTC újra rendben
    if (rtcWarning != RTC_WARNING_NONE &&
        rtcWarning != RTC_WARNING_LOST_POWER &&
        rtcWarning != RTC_WARNING_OSCILLATOR_STOP)
    {
        logEvent(
            "RTC",
            "RECOVERED",
            "",
            ""
        );

        rtcWarning = RTC_WARNING_NONE;
    }
}


// DS3231 + AT24C32 teszt
void rtcTest()
{
    logMessage("");
    logMessage("================================");
    logMessage("RTC TESZT INDUL");
    logMessage("================================");


    // DS3231 újraellenőrzése
    if (!rtc.begin())
    {
        logMessage("DS3231 RTC: HIBA!");
        return;
    }

    logMessage("DS3231 RTC: OK");


    // RTC elvesztette-e az időt
    if (rtc.lostPower())
    {
        logMessage("FIGYELEM: Az RTC elvesztette az idot!");
    }
    else
    {
        logMessage("RTC ora ervenyes.");
    }


    // Aktuális idő kiolvasása
    DateTime now = rtc.now();

    char message[100];

    snprintf(
        message,
        sizeof(message),
        "Datum: %04d-%02d-%02d",
        now.year(),
        now.month(),
        now.day()
    );

    logMessage(message);


    snprintf(
        message,
        sizeof(message),
        "Ido: %02d:%02d:%02d",
        now.hour(),
        now.minute(),
        now.second()
    );

    logMessage(message);


    logMessage("");
    logMessage("AT24C32 EEPROM TESZT");


    // AT24C32 I2C cím
    const byte EEPROM_ADDRESS = 0x57;

    // Teszt cím az EEPROM-ban
    const uint16_t TEST_ADDRESS = 0;

    // Tesztérték
    const byte TEST_VALUE = 123;


    // Írás
    Wire.beginTransmission(EEPROM_ADDRESS);

    Wire.write(highByte(TEST_ADDRESS));
    Wire.write(lowByte(TEST_ADDRESS));

    Wire.write(TEST_VALUE);

    byte error = Wire.endTransmission();


    if (error != 0)
    {
        logMessage("AT24C32 irasi hiba!");
        return;
    }

    // Az EEPROM-nak kell egy kis idő az írásra
    delay(10);


    // Kiolvasandó cím beállítása
    Wire.beginTransmission(EEPROM_ADDRESS);

    Wire.write(highByte(TEST_ADDRESS));
    Wire.write(lowByte(TEST_ADDRESS));

    error = Wire.endTransmission();


    if (error != 0)
    {
        logMessage("AT24C32 olvasasi hiba!");
        return;
    }


    // 1 byte lekérése
    Wire.requestFrom((int)EEPROM_ADDRESS, 1);


    if (Wire.available())
    {
        byte readValue = Wire.read();

        snprintf(
            message,
            sizeof(message),
            "EEPROM irt ertek: %d",
            TEST_VALUE
        );

        logMessage(message);


        snprintf(
            message,
            sizeof(message),
            "EEPROM olvasott ertek: %d",
            readValue
        );

        logMessage(message);


        if (readValue == TEST_VALUE)
        {
            logMessage("AT24C32 EEPROM: OK");
        }
        else
        {
            logMessage("AT24C32 EEPROM: HIBA - az ertek nem egyezik!");
        }
    }
    else
    {
        logMessage("AT24C32 EEPROM: nem erkezett adat!");
    }


    logMessage("================================");
    logMessage("RTC TESZT VEGE");
    logMessage("================================");
}