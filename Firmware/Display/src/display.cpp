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
    const int h = (day + (13 * (month + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
    const int weekday = (h + 5) % 7; // hétfő=0
    static const char* names[] = {"H", "K", "Sze", "Cs", "P", "Szo", "V"};
    return names[weekday];
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

static void drawStaticHouseLayout() {
    tft.fillScreen(COLOR_BG);

    // BAL OLDALI HÁZ: szélesebb, két egyforma helyiséggel.
    const int16_t leftX = 7;
    const int16_t roomY = 102;
    const int16_t roomW = 85;  // korábban 68
    const int16_t roomH = FOOTER_LINE_Y - roomY;

    const int16_t leftW = roomW * 2;

    tft.drawRect(leftX, roomY, roomW, roomH, COLOR_HOUSE);
    tft.drawRect(leftX + roomW, roomY, roomW, roomH, COLOR_HOUSE);

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

    tft.drawRect(rightX, rightY, rightW, rightH, COLOR_HOUSE);

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
    tft.fillRect(0, FOOTER_LINE_Y, SCREEN_W,
                 FOOTER_LINE_H, COLOR_LINE);
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

void displayUpdate(const MasterData& m, const LocalData& l) {
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

    drawTextField(0, 14, 119, 60, 12, roomTemp, COLOR_VALUE, COLOR_BG, 2);
    drawTextField(1, 14, 151, 60, 12, roomHumidity, COLOR_VALUE, COLOR_BG, 2);

    // A második szoba helye üres marad későbbi bővítéshez.

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

    drawTextField(2, 211, 117, 90, 12,
                  activeBranchName(m.activeBranch, m.systemState),
                  COLOR_VALUE, COLOR_BG, 1);

    drawTextField(3, 211, 133, 90, 12,
                  String("Elore ") + degreeC(flow),
                  COLOR_VALUE, COLOR_BG, 1);
    drawTextField(4, 211, 145, 90, 12,
                  String("Vissza ") + degreeC(ret),
                  COLOR_VALUE, COLOR_BG, 1);

    drawTextField(5, 211, 155, 90, 12,
                  String("Fust ") + degreeC(m.m.chimney),
                  COLOR_VALUE, COLOR_BG, 1);
    drawTextField(6, 211, 165, 90, 12,
                  String("Kaz ") + degreeC(m.m.woodBoiler),
                  COLOR_VALUE, COLOR_BG, 1);

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
    String dateText = "----.--.-- ---";
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

    drawTextField(8, 5, 200, 18, 12, dateText,
                  COLOR_VALUE, COLOR_BG, 2);
    drawTextField(9, 210, 202, 70, 22, clockText,
                  COLOR_VALUE, COLOR_BG, 2);
    drawTextField(10, 282, 208, 24, 12, secondsText,
                  COLOR_VALUE, COLOR_BG, 1);

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
