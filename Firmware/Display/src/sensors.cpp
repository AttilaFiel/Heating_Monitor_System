
#include "sensors.h"

#include <Wire.h>
#include <Adafruit_SHT31.h>

#include "config.h"
#include "rtc.h"


// ============================================================
// Globális változók
// ============================================================

static Adafruit_SHT31 sht;

static bool shtOk = false;
static uint32_t lastRead = 0;


// ============================================================
// Szenzorok inicializálása
// ============================================================

void sensorsBegin() {
    Wire.begin(PIN_SDA, PIN_SCL);

    shtOk = sht.begin(0x44) || sht.begin(0x45);

    pinMode(PIN_LDR, INPUT);

    analogReadResolution(12);
}


// ============================================================
// Szenzoradatok frissítése
// ============================================================

void sensorsUpdate(LocalData& d) {

    // --------------------------------------------------------
    // Frissítési időköz ellenőrzése
    // --------------------------------------------------------

    if (millis() - lastRead < 2000)
        return;

    lastRead = millis();


    // --------------------------------------------------------
    // SHT30 hőmérséklet- és páratartalom-mérés
    // --------------------------------------------------------

	if (shtOk) {
		// SHT30 hőmérsékletmérés és szoftveres korrekció
		d.temp = sht.readTemperature();

		if (!isnan(d.temp)) {
			d.temp += SHT30_TEMPERATURE_OFFSET_C;
		}

		// Páratartalom mérés (korrekció nélkül)
		d.humidity = sht.readHumidity();

		d.shtValid = !isnan(d.temp) && !isnan(d.humidity);
	} else {
		d.shtValid = false;
	}


    // --------------------------------------------------------
    // Fényérzékelő olvasása
    // --------------------------------------------------------

    d.photoAdc = analogRead(PIN_LDR);


    // --------------------------------------------------------
    // Sötétség érzékelése hiszterézissel
    // --------------------------------------------------------

    static bool dark = false;

    if (!dark && d.photoAdc < PHOTO_DARK_THRESHOLD) {
        dark = true;
    } else if (
        dark &&
        d.photoAdc > PHOTO_DARK_THRESHOLD + PHOTO_HYSTERESIS
    ) {
        dark = false;
    }

    d.dark = dark;


    // --------------------------------------------------------
    // RTC-adatok frissítése
    // --------------------------------------------------------

    rtcUpdate(d);
}
