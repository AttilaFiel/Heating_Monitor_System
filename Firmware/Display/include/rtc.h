#pragma once
#include "system_data.h"
void rtcBegin();
void rtcSyncNtp();
void rtcUpdate(LocalData &d);
