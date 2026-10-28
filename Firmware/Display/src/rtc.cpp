#include "rtc.h"

#include <Wire.h>
#include <RTClib.h>
#include <time.h>

#include "config.h"
#include "network.h"


// ============================================================
// Globális változók
// ============================================================

static RTC_DS3231 rtc;
static bool rtcOk = false;

// Magyarország időzónája:
// CET = UTC+1 (tél), CEST = UTC+2 (nyár)
// Az átállás az EU szabályai szerint automatikus.
static constexpr char TIME_ZONE[] =
    "CET-1CEST,M3.5.0/2,M10.5.0/3";


// ============================================================
// RTC inicializálása
// ============================================================

void rtcBegin() {
    rtcOk = rtc.begin();

    if (!rtcOk) {
        networkLog("DS3231 not detected.");
    }
}


// ============================================================
// RTC szinkronizálása NTP segítségével
// ============================================================

void rtcSyncNtp() {
    // A rendszeridő UTC-ben szinkronizálódik.
    // A helyi időzóna csak a megjelenítéskor kerül alkalmazásra.
    configTzTime(TIME_ZONE, NTP_SERVER);

    struct tm timeInfo;

    if (getLocalTime(&timeInfo, 15000)) {
        if (rtcOk) {
            // A DS3231 UTC időt tároljon, ne helyi időt.
            const time_t utcNow = time(nullptr);

            if (utcNow > 1700000000) {
                rtc.adjust(DateTime(static_cast<uint32_t>(utcNow)));
                networkLog("RTC synchronized with NTP (UTC).");
            } else {
                networkLog("NTP time invalid; RTC not adjusted.");
            }
        }
    } else {
        networkLog("NTP synchronization failed.");
    }
}


// ============================================================
// RTC-adatok frissítése
// ============================================================

void rtcUpdate(LocalData& d) {
    d.rtcAvailable = rtcOk;
    d.rtcValid = rtcOk && !rtc.lostPower();

    if (!rtcOk) {
        d.dateTime = "";
        return;
    }

    // A DS3231 UTC időt ad vissza.
    const DateTime nowUtc = rtc.now();
    const time_t utcTime = static_cast<time_t>(nowUtc.unixtime());

    // UTC -> helyi idő (Europe/Budapest), DST automatikus.
    struct tm localTimeInfo;
    localtime_r(&utcTime, &localTimeInfo);

    char b[24];

    snprintf(
        b,
        sizeof(b),
        "%04d-%02d-%02d %02d:%02d:%02d",
        localTimeInfo.tm_year + 1900,
        localTimeInfo.tm_mon + 1,
        localTimeInfo.tm_mday,
        localTimeInfo.tm_hour,
        localTimeInfo.tm_min,
        localTimeInfo.tm_sec
    );

    d.dateTime = b;
}
