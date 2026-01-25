#include "display.h"

#ifdef USE_WS2812
CRGB leds[NUM_LEDS];
#endif

int getPhysicalLedIndex(int x, int y) {
  int physicalX = (MATRIX_WIDTH - 1) - x;
  if (y % 2 == 0) {
    return y * MATRIX_WIDTH + physicalX;
  } else {
    return y * MATRIX_WIDTH + ((MATRIX_WIDTH - 1) - physicalX);
  }
}

void initDisplay(uint8_t brightness) {
#ifdef USE_WS2812
  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(brightness);
  FastLED.clear();
  FastLED.show();
  Serial.printf("✓ WS2812 ready (brightness: %d)\n", brightness);
#endif
}

void displayFrame(const Frame &f) {
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

void drawPixel(int x, int y, Pixel p) {
#ifdef USE_WS2812
  if (x >= 0 && x < MATRIX_WIDTH && y >= 0 && y < MATRIX_HEIGHT) {
    leds[getPhysicalLedIndex(x, y)] = CRGB(p.r, p.g, p.b);
  }
#endif
}

void setDisplayBrightness(uint8_t b) {
#ifdef USE_WS2812
  FastLED.setBrightness(b);
#endif
}

void showDisplay() {
#ifdef USE_WS2812
  FastLED.show();
#endif
}

void clearDisplay() {
#ifdef USE_WS2812
  FastLED.clear();
  FastLED.show();
#endif
}

void clearBuffer() {
#ifdef USE_WS2812
  FastLED.clear();
#endif
}