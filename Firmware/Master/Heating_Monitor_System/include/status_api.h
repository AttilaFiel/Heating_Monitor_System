#pragma once

#include <Arduino.h>

// A HTTP-kezeléstől független JSON-előállító függvények.
String buildLegacyStatusJson();
String buildApiStatusJson();
String buildApiHealthJson();
