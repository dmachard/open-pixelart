#ifndef CLOCK_H
#define CLOCK_H

#include "display.h"
#include <Arduino.h>

void initClock();
void updateClock();
void setClockColorIndex(uint8_t index);
void syncClockWithRTC();
void drawClock();

extern const uint8_t CLOCK_PALETTE_SIZE;

#endif // CLOCK_H
