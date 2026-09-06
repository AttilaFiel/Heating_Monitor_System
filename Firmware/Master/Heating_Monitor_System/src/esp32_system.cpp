#include <Arduino.h>
#include <esp_system.h>

#include "logger.h"
#include "esp32_system.h"

const char* resetReasonToString(esp_reset_reason_t reason)
{
    switch (reason)
    {
        case ESP_RST_POWERON:
            return "POWER_ON";

        case ESP_RST_SW:
            return "SOFTWARE";

        case ESP_RST_TASK_WDT:
        case ESP_RST_INT_WDT:
        case ESP_RST_WDT:
            return "WATCHDOG";

        case ESP_RST_BROWNOUT:
            return "BROWNOUT";

        default:
            return "UNKNOWN";
    }
}

void esp32SystemSetup()
{
    esp_reset_reason_t resetReason = esp_reset_reason();

    logEvent(
        "ESP32",
        "RESET",
        "",
        resetReasonToString(resetReason)
    );

    logEvent(
        "ESP32",
        "BOOT",
        "",
        ""
    );
}