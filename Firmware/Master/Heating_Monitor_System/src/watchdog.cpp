#include <Arduino.h>
#include <esp_task_wdt.h>

#include "watchdog.h"
#include "logger.h"
#include "network.h"

#define WATCHDOG_TIMEOUT_SECONDS 5

void watchdogSetup()
{
    esp_task_wdt_init(WATCHDOG_TIMEOUT_SECONDS, true);
    esp_task_wdt_add(NULL);
}

void watchdogUpdate()
{
    esp_task_wdt_reset();
}