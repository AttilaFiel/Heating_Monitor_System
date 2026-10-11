#pragma once

void dht11Setup();

void dht11Test();

float dht11ReadTemperature();
float dht11ReadHumidity();
bool dht11Read(float& temperature, float& humidity);