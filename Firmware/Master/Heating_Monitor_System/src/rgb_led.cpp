#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#include "config.h"
#include "system_data.h"
#include "rgb_led.h"
#include "sd_card.h"
#include "data_manager.h"
#include "network.h"


Adafruit_PWMServoDriver pwm(PCA9685_ADDRESS);

const int PWM_MAX = 4095;


// RGB LED beállítása
void setRGB(int led, int r, int g, int b)
{
    int base = 15 - (led * 3);

    pwm.setPWM(base,     0, r);  // R
    pwm.setPWM(base - 1, 0, g);  // G
    pwm.setPWM(base - 2, 0, b);  // B
}


// Minden PWM csatorna kikapcsolása
void allOff()
{
    for (int i = 0; i < 16; i++)
    {
        pwm.setPWM(i, 0, 0);
    }
}


// RGB rendszer indítása
void rgbLedSetup()
{
    // I2C
    Wire.begin(I2C_SDA, I2C_SCL);

    // PCA9685
    pwm.begin();

    // LED-ek PWM frekvenciája
    pwm.setPWMFreq(PCA9685_PWM_FREQ);

    allOff();

    // Véletlenszám-generátor inicializálása
    randomSeed(esp_random());
}


// Induló RGB teszt
void startupTest()
{
    Serial.println("PIROS");

    for (int i = 0; i < 3; i++)
    {
        setRGB(i, PWM_MAX, 0, 0);
    }

    delay(500);


    Serial.println("ZOLD");

    for (int i = 0; i < 3; i++)
    {
        setRGB(i, 0, PWM_MAX, 0);
    }

    delay(500);


    Serial.println("KEK");

    for (int i = 0; i < 3; i++)
    {
        setRGB(i, 0, 0, PWM_MAX);
    }

    delay(500);

    allOff();

    delay(100);
}


// Hullámzó fényjáték
void waveShow()
{
    Serial.println("GYORS HULLAMZO FENYJATEK");

    const int colors[][3] =
    {
        {PWM_MAX, 0, 0},
        {0, PWM_MAX, 0},
        {0, 0, PWM_MAX},
        {PWM_MAX, PWM_MAX, 0},
        {PWM_MAX, 0, PWM_MAX},
        {0, PWM_MAX, PWM_MAX},
        {PWM_MAX, PWM_MAX, PWM_MAX}
    };

    const int colorCount = 7;


    for (int cycle = 0; cycle < 12; cycle++)
    {
        int color1 = random(0, colorCount);
        int color2 = random(0, colorCount);
        int color3 = random(0, colorCount);


        // LED1
        allOff();

        setRGB(
            0,
            colors[color1][0],
            colors[color1][1],
            colors[color1][2]
        );

        delay(150);


        // LED1 + LED2
        setRGB(
            1,
            colors[color2][0],
            colors[color2][1],
            colors[color2][2]
        );

        delay(150);


        // LED2 + LED3
        setRGB(0, 0, 0, 0);

        setRGB(
            2,
            colors[color3][0],
            colors[color3][1],
            colors[color3][2]
        );

        delay(150);


        // LED3
        setRGB(1, 0, 0, 0);

        delay(150);


        allOff();

        delay(100);
    }

    allOff();

    delay(300);
}


// Teljes káosz
void chaosShow()
{
    Serial.println("TELJES KAOSZ!");

    unsigned long startTime = millis();

    while (millis() - startTime < 5000)
    {
        for (int led = 0; led < 3; led++)
        {
            int r = random(0, PWM_MAX + 1);
            int g = random(0, PWM_MAX + 1);
            int b = random(0, PWM_MAX + 1);

            setRGB(led, r, g, b);
        }

        delay(random(30, 100));
    }

    allOff();

    delay(300);
}


// Normál véletlen LED mód
void randomLedShow()
{
    int r1 = random(0, PWM_MAX + 1);
    int g1 = random(0, PWM_MAX + 1);
    int b1 = random(0, PWM_MAX + 1);

    int r2 = random(0, PWM_MAX + 1);
    int g2 = random(0, PWM_MAX + 1);
    int b2 = random(0, PWM_MAX + 1);

    int r3 = random(0, PWM_MAX + 1);
    int g3 = random(0, PWM_MAX + 1);
    int b3 = random(0, PWM_MAX + 1);


    setRGB(0, r1, g1, b1);
    setRGB(1, r2, g2, b2);
    setRGB(2, r3, g3, b3);


    delay(random(100, 300));
}

void rgbLedUpdate()
{
    // LED 1 – Gáz ág
    if (systemData.alarmReason == ALARM_REASON_BOTH_BRANCHES)
    {
        setRGB(0, PWM_MAX, 0, 0);
    }
    else if (systemData.activeBranch == HEATING_GAS)
    {
        setRGB(0, 0, PWM_MAX, 0);
    }
    else
    {
        setRGB(0, 0, 0, 0);
    }


    // LED 2 – Fa ág
    if (systemData.alarmReason == ALARM_REASON_BOTH_BRANCHES)
    {
        // Piros – mindkét ág aktív
        setRGB(1, PWM_MAX, 0, 0);
    }
    else if (systemData.activeBranch == HEATING_WOOD)
    {
        if (systemData.alarmReason == ALARM_REASON_FIRE_WITHOUT_CIRCULATOR ||
            systemData.alarmReason == ALARM_REASON_WOOD_BOILER_OVERHEAT ||
            systemData.alarmReason == ALARM_REASON_STARTUP_FAULT)
        {
            // Piros
            setRGB(1, PWM_MAX, 0, 0);
        }
        else if (systemData.alarmReason == ALARM_REASON_WOOD_BOILER_WARNING ||
                systemData.alarmReason == ALARM_REASON_STARTUP_WARNING)
        {
            // Narancssárga
            setRGB(1, PWM_MAX, PWM_MAX / 3, 0);
        }
        else if (!systemData.firePresent)
        {
            // Kék – aktív fa ág, de nincs tűz
            setRGB(1, 0, 0, PWM_MAX);
        }
        else
        {
            // Zöld – aktív fa üzem, minden rendben
            setRGB(1, 0, PWM_MAX, 0);
        }
    }
    else
    {
        setRGB(1, 0, 0, 0);
    }

    // LED 3 – ESP32 státusz
    if (systemData.alarmState == ALARM_CRITICAL)
    {
        // Piros – kritikus hiba
        setRGB(2, PWM_MAX, 0, 0);
    }
    else if (systemData.alarmState == ALARM_WARNING)
    {
        // Narancssárga – figyelmeztetés
        setRGB(2, PWM_MAX, PWM_MAX / 3, 0);
    }
    else if (isStartupActive())
    {
        // Kék – startup
        setRGB(2, 0, 0, PWM_MAX);
    }
    else if (otaUpdateActive())
    {
        // Lila – OTA frissítés
        setRGB(2, PWM_MAX, 0, PWM_MAX);
    }
    else
    {
        // Zöld – minden rendben
        setRGB(2, 0, PWM_MAX, 0);
    }
}