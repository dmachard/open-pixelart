#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"
#include "decoder.h"

#ifdef USE_WS2812
    #include <FastLED.h>
    CRGB leds[NUM_LEDS];
#endif

// Convert logical (x,y) coordinates to physical LED strip index
// for a serpentine-wired matrix like WS2812
inline int getPhysicalLedIndex(int x, int y) {
    int physicalX = (MATRIX_WIDTH - 1) - x;
    if (y % 2 == 0) {
        return y * MATRIX_WIDTH + physicalX;
    } else {
        return y * MATRIX_WIDTH + ((MATRIX_WIDTH - 1) - physicalX);
    }
}

// Initialize display
void initDisplay() {
#ifdef USE_WS2812
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(25);
    FastLED.clear();
    FastLED.show();
    Serial.println("✓ WS2812 ready");
#endif
}

// Display a frame on the LED matrix
inline void displayFrame(const Frame &f) {
#ifdef USE_WS2812
    FastLED.setBrightness(f.brightness);
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            const Pixel &p = f.pixels[y][x];
            leds[getPhysicalLedIndex(x, y)] = CRGB(p.r, p.g, p.b);
        }
    }
    FastLED.show();
#endif
}

// Clear the display
inline void clearDisplay() {
#ifdef USE_WS2812
    FastLED.clear();
    FastLED.show();
#endif
}

#endif