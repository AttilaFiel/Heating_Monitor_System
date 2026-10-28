#pragma once
#include "system_data.h"
#include "watchdog.h"

void networkBegin();
void networkLoop();
void networkLog(const String &s);
void networkSetLocalData(const LocalData &data);
bool networkFetchMaster(MasterData &m);
