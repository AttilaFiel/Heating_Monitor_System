#pragma once

#include <RTClib.h>

extern RTC_DS3231 rtc;

void rtcSetup();
void rtcUpdate();
bool rtcIsAvailable();
void rtcTest();