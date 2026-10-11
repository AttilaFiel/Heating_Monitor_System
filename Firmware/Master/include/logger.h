#pragma once

enum LoggerWriteErrorReason
{
    LOGGER_WRITE_ERROR_NONE,
    LOGGER_WRITE_ERROR_EVENT_LOG,
    LOGGER_WRITE_ERROR_MEASUREMENT_LOG
};

void loggerSetup();

bool logEvent(
    const char* type,
    const char* event,
    const char* oldValue,
    const char* newValue
);

bool loggerHasWriteError();
LoggerWriteErrorReason loggerGetWriteErrorReason();
void loggerClearWriteError();

bool logMeasurements();
void loggerUpdate();