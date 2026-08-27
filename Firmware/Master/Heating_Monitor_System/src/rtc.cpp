#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>

#include "network.h"
#include "rtc.h"


RTC_DS3231 rtc;


// RTC inicializálása
void rtcSetup()
{
    if (!rtc.begin())
    {
        logMessage("DS3231 RTC HIBA: nem inicializalhato!");
        return;
    }

    logMessage("DS3231 RTC inicializalva.");

    if (rtc.lostPower())
    {
        logMessage("RTC elvesztette az idot - ido beallitasa.");

        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

        logMessage("RTC ido beallitva.");
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