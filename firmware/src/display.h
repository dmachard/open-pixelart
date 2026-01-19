#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"
#include "decoder.h"
#include <Arduino.h>

#ifdef USE_WS2812
#include <FastLED.h>
extern CRGB leds[NUM_LEDS];
#endif

int getPhysicalLedIndex(int x, int y);
void initDisplay(uint8_t brightness);
void displayFrame(const Frame &f);
void drawPixel(int x, int y, Pixel p);
void setDisplayBrightness(uint8_t b);
void clearDisplay();
void showDisplay();

#endif