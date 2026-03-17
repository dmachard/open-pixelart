#ifndef NIGHTLIGHT_H
#define NIGHTLIGHT_H

#include "decoder.h"
#include <Arduino.h>

struct NightLightConfig {
  bool enabled;
  uint8_t startHour;
  uint8_t startMinute;
  uint8_t endHour;
  uint8_t endMinute;
  uint8_t colorIndex; // 0=Orange chaud, 1=Rouge, 2=Violet, 3=Blanc chaud
  uint8_t brightness; // 1 – 20
};

void initNightLight();
void setNightLightConfig(const NightLightConfig &cfg);
const NightLightConfig &getNightLightConfig();

// Returns true if the given time falls inside the configured night window.
// Handles schedules that cross midnight (e.g. 22:00 → 06:00).
bool isNightLightTime(uint8_t hour, uint8_t minute);

// Draw the night-light pattern and push it to the display.
void drawNightLight();

#endif // NIGHTLIGHT_H
