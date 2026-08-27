#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "network.h"
#include "i2c_scanner.h"


void i2cSetup()
{
    Wire.begin(I2C_SDA, I2C_SCL);

    logMessage("I2C busz inicializalva.");
}


void scanI2C()
{
    logMessage("");
    logMessage("================================");
    logMessage("I2C KERESES INDUL");
    logMessage("================================");

    int deviceCount = 0;

    for (byte address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);

        byte error = Wire.endTransmission();

        if (error == 0)
        {
            char message[50];

            snprintf(
                message,
                sizeof(message),
                "I2C eszkoz talalva: 0x%02X",
                address
            );

            logMessage(message);

            deviceCount++;
        }
    }

    char result[50];

    snprintf(
        result,
        sizeof(result),
        "Talalt I2C eszkozok szama: %d",
        deviceCount
    );

    logMessage(result);

    logMessage("================================");
    logMessage("I2C KERESES VEGE");
    logMessage("================================");
    logMessage("");
}