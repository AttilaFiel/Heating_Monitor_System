#include "display.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include "config.h"
#include "network.h"

static Adafruit_ILI9341 tft(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RESET);
static XPT2046_Touchscreen touch(PIN_TOUCH_CS, PIN_TOUCH_IRQ);
static uint32_t lastTouch = 0;

static constexpr int16_t SCREEN_W = 320;
static constexpr int16_t FOOTER_LINE_Y = 189;
static constexpr int16_t FOOTER_LINE_H = 3;

static constexpr uint16_t COLOR_BG = ILI9341_BLACK;
static constexpr uint16_t COLOR_LINE = ILI9341_WHITE;
static constexpr uint16_t COLOR_HOUSE = ILI9341_CYAN;
static constexpr uint16_t COLOR_VALUE = ILI9341_WHITE;
static constexpr uint16_t COLOR_WARNING = ILI9341_ORANGE;

// ------------------------------------------------------------
// Gyorsítótárazott mezők: csak megváltozott adatot rajzolunk újra.
// ------------------------------------------------------------

struct TextFieldCache {
    int16_t x = 0, y = 0, width = 0, height = 0;
    String text;
    uint16_t textColor = ILI9341_WHITE;
    uint16_t backgroundColor = ILI9341_BLACK;
    uint8_t textSize = 1;
    bool initialized = false;
};

static constexpr uint8_t MAX_CACHED_FIELDS = 16;
static TextFieldCache fieldCache[MAX_CACHED_FIELDS];

static String tt(float value) {
    return isnan(value) ? "--.-" : String(value, 1);
}

static String degreeC(float value) {
    // A fokjel az Adafruit GFX alap betűkészletében a 0xF7 karakter.
    return tt(value) + String((char)0xF7) + "C";
}

static String activeBranchName(const String& branch, const String& systemState) {
    if (branch == "HEATING_GAS" || systemState == "SYSTEM_GAS") return "GAZAG";
    if (branch == "HEATING_WOOD" || systemState == "SYSTEM_WOOD") return "FAAG";
    if (branch == "HEATING_NONE" || systemState == "SYSTEM_OFF") return "NINCS AKTIV AG";
    if (branch == "HEATING_ERROR" || systemState == "SYSTEM_FAULT") return "AGHIBA";
    if (systemState == "SYSTEM_STARTUP") return "INDULAS";
    if (systemState == "SYSTEM_FIRE_WITHOUT_CIRCULATOR") return "TUZ / NINCS KERINGES";
    return "ISMERETLEN AG";
}

static String weekdayHu(int year, int month, int day) {
    // Zeller-képlet: 0=szombat, 1=vasárnap, 2=hétfő...
    if (month < 3) {
        month += 12;
        --year;
    }

    const int k = year % 100;
    const int j = year / 100;

    const int h = (
        day +
        (13 * (month + 1)) / 5 +
        k +
        k / 4 +
        j / 4 +
        5 * j
    ) % 7;

    static const char* names[] = {
        "Szo", "V", "H", "K", "Sze", "Cs", "P"
    };

    return names[h];
}

static void drawThickRect(
    int16_t x,
    int16_t y,
    int16_t width,
    int16_t height,
    uint16_t color,
    uint8_t thickness
) {
    for (uint8_t i = 0; i < thickness; i++) {
        tft.drawRect(
            x + i,
            y + i,
            width - 2 * i,
            height - 2 * i,
            color
        );
    }
}

static void drawTextField(
    uint8_t cacheId,
    int16_t x, int16_t y, int16_t width, int16_t height,
    const String& text,
    uint16_t textColor = COLOR_VALUE,
    uint16_t backgroundColor = COLOR_BG,
    uint8_t textSize = 1
) {
    if (cacheId >= MAX_CACHED_FIELDS) return;
    TextFieldCache& cache = fieldCache[cacheId];

    const bool changed =
        !cache.initialized ||
        cache.x != x || cache.y != y ||
        cache.width != width || cache.height != height ||
        cache.text != text ||
        cache.textColor != textColor ||
        cache.backgroundColor != backgroundColor ||
        cache.textSize != textSize;

    if (!changed) return;

    tft.fillRect(x, y, width, height, backgroundColor);
    tft.setTextColor(textColor);
    tft.setTextSize(textSize);
    tft.setCursor(x, y);
    tft.print(text);

    cache.x = x;
    cache.y = y;
    cache.width = width;
    cache.height = height;
    cache.text = text;
    cache.textColor = textColor;
    cache.backgroundColor = backgroundColor;
    cache.textSize = textSize;
    cache.initialized = true;
}

// ============================================================
// Hőmérséklet megjelenítése eltérő betűméretekkel
// Egyetlen TextFieldCache használatával
// ============================================================

static void drawTemperature(
    uint8_t cacheId,
    int16_t x,
    int16_t y,
    float temperature
) {
    if (cacheId >= MAX_CACHED_FIELDS) return;

    // Hőmérséklet formázása egy tizedesjeggyel
    const String temp = isnan(temperature)
        ? "--.-"
        : String(temperature, 1);

    // Cache ellenőrzése
    TextFieldCache& cache = fieldCache[cacheId];

    const bool changed =
        !cache.initialized ||
        cache.x != x ||
        cache.y != y ||
        cache.text != temp;

    // Ha nem változott az érték vagy a pozíció, nincs rajzolás
    if (!changed) return;

    // Teljes hőmérséklet-megjelenítési terület törlése
    const int16_t fieldWidth = 70;
    const int16_t fieldHeight = 32;

    tft.fillRect(
        x,
        y,
        fieldWidth,
        fieldHeight,
        COLOR_BG
    );

    // Egész szám és tizedes rész szétválasztása
    const int dotPos = temp.indexOf('.');

    const String wholePart = temp.substring(0, dotPos);
    const String decimalPart = temp.substring(dotPos);

    // Egész szám szélessége 4-es betűmérettel
    const int16_t wholeWidth = wholePart.length() * 24;

    // Egész szám: nagy betűméret
    tft.setTextColor(COLOR_VALUE);
    tft.setTextSize(4);
    tft.setCursor(x, y);
    tft.print(wholePart);

    // Tizedes rész: kisebb betűméret, alulra igazítva
    tft.setTextSize(2);
    tft.setCursor(x + wholeWidth, y + 16);
    tft.print(decimalPart);

    // °C: kisebb betűméret, felülre igazítva
    tft.setCursor(x + wholeWidth, y);
    tft.print(String((char)0xF7) + "C");

    // Cache frissítése
    cache.x = x;
    cache.y = y;
    cache.width = fieldWidth;
    cache.height = fieldHeight;
    cache.text = temp;
    cache.textColor = COLOR_VALUE;
    cache.backgroundColor = COLOR_BG;
    cache.textSize = 4;
    cache.initialized = true;
}

// ============================================================
// Páratartalom megjelenítése eltérő betűméretekkel
// Egyetlen TextFieldCache használatával
// ============================================================

static void drawHumidity(
    uint8_t cacheId,
    int16_t x,
    int16_t y,
    float humidity
) {
    if (cacheId >= MAX_CACHED_FIELDS) return;

     // Egész páratartalom meghatározása
    const int humidityInt = isnan(humidity)
        ? -999
        : (int)humidity;

    // Cache ellenőrzése
    TextFieldCache& cache = fieldCache[cacheId];

    const String humidityText = humidityInt == -999
        ? "--"
        : String(humidityInt);

    const bool changed =
        !cache.initialized ||
        cache.x != x ||
        cache.y != y ||
        cache.text != humidityText;

    // Ha nem változott az érték vagy a pozíció, nincs rajzolás
    if (!changed) return;

    // Teljes páratartalom-megjelenítési terület törlése
    const int16_t fieldWidth = 70;
    const int16_t fieldHeight = 32;

    tft.fillRect(
        x,
        y,
        fieldWidth,
        fieldHeight,
        COLOR_BG
    );

    // Egész érték
    const int16_t wholeWidth = humidityText.length() * 24;

    // Egész szám: nagy betűméret
    tft.setTextColor(COLOR_VALUE);
    tft.setTextSize(4);
    tft.setCursor(x, y);
    tft.print(humidityText);

    // Tizedes rész: kisebb betűméret, alulra igazítva
    // tft.setTextSize(2);
    // tft.setCursor(x + wholeWidth, y + 16);
    // tft.print(decimalPart);

    // %: kisebb betűméret, felülre igazítva
    tft.setTextSize(3);
    tft.setCursor(x + wholeWidth + 8, y + 8);
    tft.print("%");

    // Cache frissítése
    cache.x = x;
    cache.y = y;
    cache.width = fieldWidth;
    cache.height = fieldHeight;
    cache.text = humidityText;
    cache.textColor = COLOR_VALUE;
    cache.backgroundColor = COLOR_BG;
    cache.textSize = 4;
    cache.initialized = true;
}

static void drawStaticHouseLayout() {
    tft.fillScreen(COLOR_BG);

    // BAL OLDALI HÁZ: szélesebb, két egyforma helyiséggel.
    const int16_t leftX = 7;
    const int16_t roomY = 102;
    const int16_t roomW = 92;
    const int16_t roomH = FOOTER_LINE_Y - roomY;

    const int16_t leftW = roomW * 2;

    drawThickRect(leftX, roomY, roomW, roomH, COLOR_HOUSE, 2);
    drawThickRect(leftX + roomW, roomY, roomW, roomH, COLOR_HOUSE, 2);

    // Bal ház tetője: csúcs a két helyiség határán.
    const int16_t peakX = leftX + roomW;
    const int16_t peakY = 19;

    tft.drawLine(leftX, roomY, peakX, peakY, COLOR_HOUSE);
    tft.drawLine(peakX, peakY, leftX + leftW, roomY, COLOR_HOUSE);

    tft.drawLine(leftX - 5, roomY + 1,
                 peakX, peakY - 3, COLOR_LINE);
    tft.drawLine(peakX, peakY - 3,
                 leftX + leftW + 5, roomY + 1, COLOR_LINE);
    tft.drawLine(leftX - 5, roomY + 2,
                 leftX + leftW + 5, roomY + 2, COLOR_LINE);

    // JOBB OLDALI HÁZ: 25%-kal keskenyebb.
    const int16_t rightX = 203;
    const int16_t rightY = 112;
    const int16_t rightW = 110;  // korábban 147
    const int16_t rightH = FOOTER_LINE_Y - rightY;

    drawThickRect(rightX, rightY, rightW, rightH, COLOR_HOUSE, 2);

    // Lapos tető, az új házszélességhez igazítva.
    const int16_t rightPeakX = rightX + rightW / 2;
    const int16_t rightPeakY = rightY - 18;

    tft.drawLine(rightX - 5, rightY,
                 rightPeakX, rightPeakY, COLOR_LINE);
    tft.drawLine(rightPeakX, rightPeakY,
                 rightX + rightW + 5, rightY, COLOR_LINE);
    tft.drawLine(rightX - 5, rightY + 2,
                 rightX + rightW + 5, rightY + 2, COLOR_LINE);

    // A 3 pixeles elválasztó a házak alapvonala.
    drawThickRect(0, FOOTER_LINE_Y, SCREEN_W, FOOTER_LINE_H, COLOR_LINE, 2);
}

void displayBegin() {
    // TFT háttérvilágítás PWM inicializálása
    const uint8_t backlightChannel = 0;

    ledcSetup(backlightChannel, 5000, 8);
    ledcAttachPin(PIN_TFT_LED, backlightChannel);

    const uint8_t brightness =
        (BACKLIGHT_BRIGHTNESS_PERCENT * 255) / 100;

    ledcWrite(backlightChannel, brightness);

    tft.begin(10000000);  // 10 MHz
    tft.setRotation(1);
    touch.begin();
    touch.setRotation(1);
    pinMode(PIN_TOUCH_IRQ, INPUT);
    drawStaticHouseLayout();
}

void displayMessage(const String& a, const String& b) {
    tft.fillScreen(COLOR_BG);
    for (uint8_t i = 0; i < MAX_CACHED_FIELDS; ++i) {
        fieldCache[i].initialized = false;
    }
    tft.setTextColor(ILI9341_CYAN);
    tft.setTextSize(2);
    tft.setCursor(8, 10);
    tft.println(a);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.setCursor(8, 45);
    tft.println(b);
}

void displayUpdate(
    const MasterData& m,
    const LocalData& l,
    const KidsThermometerData& kids
) {
    // Háttérvilágítás fényerejének beállítása
    const uint8_t brightnessPercent = l.dark
        ? BACKLIGHT_DARK_PERCENT
        : BACKLIGHT_BRIGHTNESS_PERCENT;

    const uint8_t brightness =
        (brightnessPercent * 255) / 100;

    ledcWrite(0, brightness);

    // Érintés naplózása; a képernyőre nem rajzolunk tesztpontot.
    if (touch.touched() && millis() - lastTouch > TOUCH_DEBOUNCE_MS) {
        lastTouch = millis();
        TS_Point p = touch.getPoint();
        networkLog("Touch raw " + String(p.x) + "," + String(p.y) + "," + String(p.z));
    }

    // BAL HÁZ: csak a Display saját SHT30 mérési értékei.
    const String roomTemp = l.shtValid ? degreeC(l.temp) : String("--.-") + String((char)0xF7) + "C";
    const String roomHumidity = l.shtValid ? tt(l.humidity) + "%" : "--.-%";

    // Karakter szélessége: 6 pixel
    // Karakter magassága: 8 pixel
    // drawTemperature paraméterek jelentése:
    // Paraméter	Érték	    Jelentés
    // 1.	        0   	    A mező azonosítója. Ha a függvény kezeli a mezőket, ez alapján tudja azonosítani a korábban kirajzolt mezőt.
    // 2.	        14	        A mező bal felső sarkának X koordinátája.
    // 3.	        119	        A mező bal felső sarkának Y koordinátája.
    // 4.	        l.temp      A kiírandó szöveg, vagyis a hőmérsékleti változó értéke.

    drawTemperature(
        0,
        17,
        115,
        l.temp
    );

    drawHumidity(
        1,
        17,
        151,
        l.humidity
    );

    // --------------------------------------------------------
    // JOBB szoba: HMS Kids Thermometer adatai
    // --------------------------------------------------------

    // Hőmérséklet
    const String kidsTemp =
        kids.valid
            ? degreeC(kids.temperature)
            : String("--.-") + String((char)0xF7) + "C";

    // Páratartalom
    const String kidsHumidity =
        kids.valid
            ? tt(kids.humidity) + "%"
            : "--.-%";

    // Hőmérséklet
    drawTemperature(
        2,
        110,
        115,
        kids.temperature
    );

    // Páratartalom
    drawHumidity(
        3,
        110,
        151,
        kids.humidity
    );

    // JOBB HÁZ: az aktív ág előremenő és visszatérő adatai.
    const bool gasActive =
        m.activeBranch == "HEATING_GAS" || m.systemState == "SYSTEM_GAS";
    const bool woodActive =
        m.activeBranch == "HEATING_WOOD" || m.systemState == "SYSTEM_WOOD";

    float flow = NAN;
    float ret = NAN;
    if (gasActive) {
        flow = m.m.gasFlow;
        ret = m.m.gasReturn;
    } else if (woodActive) {
        flow = m.m.woodFlow;
        ret = m.m.woodReturn;
    }

    // Karakter szélessége: 6 pixel
    // Karakter magassága: 8 pixel
    // drawTextField paraméterek jelentése:
    // Paraméter	Érték	    Jelentés
    // 1.	        3   	    A mező azonosítója. Ha a függvény kezeli a mezőket, ez alapján tudja azonosítani a korábban kirajzolt mezőt.
    // 2.	        211	        A mező bal felső sarkának X koordinátája.
    // 3.	        133	        A mező bal felső sarkának Y koordinátája.
    // 4.	        90  	    A mező szélessége pixelben.
    // 5.	        12	        A mező magassága pixelben.
    // 6.	        kidsTemp    A kiírandó szöveg, vagyis a Kids hőmérsékleti értéke.
    // 7.	        COLOR_VALUE A szöveg színe.
    // 8.	        COLOR_BG	A mező háttérszíne.
    // 9.	        2       	A szöveg mérete vagy skálázása — a pontos jelentés a drawTextField() függvény definíciójától függ.

    drawTextField(
        4,
        213, 120, 90, 8,
        activeBranchName(m.activeBranch, m.systemState),
        COLOR_VALUE,
        COLOR_BG,
        1
    );

    drawTextField(
        5,
        213, 138, 90, 8,
        String("Elore ") + degreeC(flow),
        COLOR_VALUE,
        COLOR_BG,
        1
    );
    drawTextField(
        6,
        213, 149, 90, 8,
        String("Vissza ") + degreeC(ret),
        COLOR_VALUE,
        COLOR_BG,
        1
    );
    drawTextField(
        7,
        213, 161, 90, 8,
        String("Fust ") + degreeC(m.m.chimney),
        COLOR_VALUE,
        COLOR_BG,
        1
    );
    drawTextField(
        8,
        213, 173, 90, 8,
        String("Kaz ") + degreeC(m.m.woodBoiler),
        COLOR_VALUE,
        COLOR_BG,
        1
    );

    // Master utolsó mérési frissítésének időpontja: YYYY.MM.DD HH:MM.
    /*String updateTime = "----.--.-- --:--";
    if (m.timestamp.length() >= 16) {
        updateTime = m.timestamp.substring(0, 10);
        updateTime.replace('-', '.');
        updateTime += " ";
        updateTime += m.timestamp.substring(11, 16);
    }
    drawTextField(7, 171, 161, 140, 12, updateTime,
                  COLOR_VALUE, COLOR_BG, 1);*/

    // ALSÓ ÖTÖD: dátum balra, óra jobbra, másodperc fél méretben.
    String dateText = "--.--.-- ---";
    String clockText = "--:--";
    String secondsText = "--";

    if (l.dateTime.length() >= 19) {
        const int year = l.dateTime.substring(0, 4).toInt();
        const int month = l.dateTime.substring(5, 7).toInt();
        const int day = l.dateTime.substring(8, 10).toInt();

        dateText = l.dateTime.substring(0, 4) + "." +
                   l.dateTime.substring(5, 7) + "." +
                   l.dateTime.substring(8, 10) + " " +
                   weekdayHu(year, month, day);

        clockText = l.dateTime.substring(11, 16);
        secondsText = l.dateTime.substring(17, 19);
    }

    drawTextField(
        9,
        5, 202, 168, 16,
        dateText,
        COLOR_VALUE,
        COLOR_BG,
        2
    );
    drawTextField(
        10,
        190, 202, 120, 32,
        clockText,
        COLOR_VALUE,
        COLOR_BG,
        4
    );
    // drawTextField(
    //     11,
    //     285, 217, 30, 16,
    //     secondsText,
    //     COLOR_VALUE,
    //     COLOR_BG,
    //     2
    // );

    // Hibánál a felső sáv elfedi a normál kép tetejét.
    // Master-riasztáshoz később külön, teljes képernyős nézet készülhet.
    const bool displayError = !l.shtValid || !l.rtcAvailable || !l.rtcValid;
    static bool previousDisplayError = false;

    if (displayError != previousDisplayError) {
        if (displayError) {
            tft.fillRect(0, 0, SCREEN_W, 22, COLOR_WARNING);
            tft.setTextColor(ILI9341_BLACK);
            tft.setTextSize(1);
            tft.setCursor(5, 7);
            if (!l.shtValid) {
                tft.print("DISPLAY SENSOR ERROR");
            } else {
                tft.print("DISPLAY RTC ERROR");
            }
        } else {
            drawStaticHouseLayout();
            for (uint8_t i = 0; i < MAX_CACHED_FIELDS; ++i) {
                fieldCache[i].initialized = false;
            }
        }
        previousDisplayError = displayError;
    }
}
