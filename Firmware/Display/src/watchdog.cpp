
#include "watchdog.h"

#include <esp_task_wdt.h>

#include "config.h"


// ============================================================
// Watchdog inicializálása
// ============================================================

void watchdogBegin() {
    esp_task_wdt_init(
        WATCHDOG_TIMEOUT_SECONDS,
        true
    );

    esp_task_wdt_add(nullptr);
}


// ============================================================
// Watchdog etetése
// ============================================================

void watchdogFeed() {
    esp_task_wdt_reset();
}
