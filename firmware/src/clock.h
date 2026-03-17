#ifndef CLOCK_H
#define CLOCK_H

#include "display.h"
#include <Arduino.h>

void initClock();
void updateClock();
void setClockColorIndex(uint8_t index);
void setClockGradientIndex(uint8_t index);
void syncClockWithRTC();
void drawClock();

// Returns the current h/m/s tracked by the clock module.
void getCurrentTime(uint8_t &h, uint8_t &m, uint8_t &s);

extern const uint8_t CLOCK_PALETTE_SIZE;

#endif // CLOCK_H
