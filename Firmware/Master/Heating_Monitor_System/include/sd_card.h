#pragma once

void sdCardSetup();
void sdCardTest();

void sdCardUpdate();
bool sdCardIsAvailable();
bool sdCardIsFull();
bool sdCardHasWarning();

bool sdCardIsBusy();
void sdCardSetBusy(bool busy);