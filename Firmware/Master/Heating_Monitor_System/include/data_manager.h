#pragma once

#include "system_data.h"


// --------------------------------
// Data Manager inicializalasa
// --------------------------------
void dataManagerSetup();

// --------------------------------
// Adatok feldolgozasa
// --------------------------------
void dataManagerUpdate();

// --------------------------------
// Data Manager teszt
// --------------------------------
void dataManagerTest();

// --------------------------------
// Rendszerallapot frissitese
// --------------------------------
void updateSystemState();

// --------------------------------
// Riasztasi allapot frissitese
// --------------------------------
void updateAlarmState();

// --------------------------------
// Rendszerallapot szoveges reprezentacioja
// --------------------------------
const char* systemStateToLogString(SystemState state);

// --------------------------------
// Fűtési ág állapot logolása string formátumban
// --------------------------------
const char* heatingBranchToLogString(HeatingBranch branch);

// --------------------------------
// Riasztasi allapot szoveges reprezentacioja
// --------------------------------
const char* alarmStateToLogString(AlarmState state);

// --------------------------------
// Startup allapot lekerdezese
// --------------------------------
bool isStartupActive();