
#include "outputs.h"

#include <Arduino.h>

#include "config.h"


// ============================================================
// Segédfüggvények
// ============================================================

// Százalék átalakítása 0–255 közötti PWM értékre
static uint8_t duty(uint8_t p) {
    return (uint16_t)p * 255 / 100;
}


// RGB LED vezérlése
static void rgb(bool r, bool g, bool b, uint8_t p) {
    ledcWrite(1, r ? duty(p) : 0);
    ledcWrite(2, g ? duty(p) : 0);
    ledcWrite(3, b ? duty(p) : 0);
}


// ============================================================
// Globális változók
// ============================================================

static uint32_t alarmStart = 0;


// ============================================================
// Kimenetek inicializálása
// ============================================================

void outputsBegin() {
    // Hangjelzők konfigurálása
    pinMode(PIN_BUZZER_1, OUTPUT);
    pinMode(PIN_BUZZER_2, OUTPUT);

    digitalWrite(PIN_BUZZER_1, LOW);
    digitalWrite(PIN_BUZZER_2, LOW);


    // Piros RGB csatorna
    ledcSetup(1, 5000, 8);
    ledcAttachPin(PIN_LED_R, 1);


    // Zöld RGB csatorna
    ledcSetup(2, 5000, 8);
    ledcAttachPin(PIN_LED_G, 2);


    // Kék RGB csatorna
    ledcSetup(3, 5000, 8);
    ledcAttachPin(PIN_LED_B, 3);
}


// ============================================================
// Kimenetek frissítése
// ============================================================

void outputsUpdate(
    const String& s,
    const String& branch,
    const String& a,
    bool dark
) {
    // RGB LED fényerejének kiválasztása
    uint8_t p = dark
        ? RGB_DARK_PERCENT
        : RGB_BRIGHTNESS_PERCENT;

    if (a == "ALARM_WARNING" || a == "ALARM_CRITICAL") {
        p = RGB_ALARM_PERCENT;
    }


    // --------------------------------------------------------
    // Kritikus riasztás
    // --------------------------------------------------------

    if (a == "ALARM_CRITICAL") {
        rgb(true, false, false, p);

        digitalWrite(PIN_BUZZER_1, LOW);

        digitalWrite(
            PIN_BUZZER_2,
            (millis() % 1000) < 500 ? HIGH : LOW
        );

        return;
    }


    // --------------------------------------------------------
    // Figyelmeztető riasztás
    // --------------------------------------------------------

    if (a == "ALARM_WARNING") {
        rgb(true, true, false, p);

        digitalWrite(PIN_BUZZER_2, LOW);

        digitalWrite(
            PIN_BUZZER_1,
            (millis() % 1250) < 250 ? HIGH : LOW
        );

        return;
    }


    // --------------------------------------------------------
    // Nincs aktív riasztás
    // --------------------------------------------------------

    digitalWrite(PIN_BUZZER_1, LOW);
    digitalWrite(PIN_BUZZER_2, LOW);


    // --------------------------------------------------------
    // Üzemi állapot szerinti RGB LED
    // --------------------------------------------------------

    if (
        s == "SYSTEM_STARTUP" ||
        s == "SYSTEM_FIRE_WITHOUT_CIRCULATOR"
    ) {
        rgb(true, false, true, p);
    }
    else if (
        branch == "HEATING_GAS" ||
        s == "SYSTEM_GAS"
    ) {
        rgb(false, false, true, p);
    }
    else if (
        branch == "HEATING_WOOD" ||
        s == "SYSTEM_WOOD"
    ) {
        rgb(false, true, false, p);
    }
    else {
        rgb(false, false, false, p);
    }
}
