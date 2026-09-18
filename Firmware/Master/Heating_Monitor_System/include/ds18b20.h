#pragma once

#include <OneWire.h>
#include <DallasTemperature.h>

void ds18b20Setup();

void ds18b20Scan();

extern DallasTemperature ds18b20;

bool stringToAddress(const char* addressString, DeviceAddress address);

float readSensorTemperature(const char* sensorAddress);

void updateSensorReadStats();