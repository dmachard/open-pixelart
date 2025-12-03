#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"
#include "decoder.h"

#ifdef USE_WS2812
    #include <FastLED.h>
    extern CRGB leds[NUM_LEDS];
#endif

int getPhysicalLedIndex(int x, int y);
void initDisplay();
void displayFrame(const Frame &f);
void clearDisplay();

#endif