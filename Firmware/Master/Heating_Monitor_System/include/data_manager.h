#pragma once

#include "system_data.h"


// --------------------------------
// Data Manager inicializálása
// --------------------------------
void dataManagerSetup();

// --------------------------------
// Adatok feldolgozása
// --------------------------------
void dataManagerUpdate();

// --------------------------------
// Data Manager teszt
// --------------------------------
void dataManagerTest();

// --------------------------------
// Rendszerállapot frissítése
// --------------------------------
void updateSystemState();

// --------------------------------
// Riasztási állapot frissítése
// --------------------------------
void updateAlarmState();

// --------------------------------
// Rendszerállapot szöveges reprezentációja
// --------------------------------
const char* systemStateToLogString(SystemState state);

// --------------------------------
// Fűtési ág állapot logolása string formátumban
// --------------------------------
const char* heatingBranchToLogString(HeatingBranch branch);

// --------------------------------
// Riasztási állapot szöveges reprezentációja
// --------------------------------
const char* alarmStateToLogString(AlarmState state);

// --------------------------------
// Startup állapot lekérdezése
// --------------------------------
bool isStartupActive();