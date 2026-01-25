#include "audio.h"

static uint8_t currentSpectrum[16] = {0};

static uint8_t currentStyle = 0; // 0 = Bars, 1 = Radial

void initAudio() {
  for (int i = 0; i < 16; i++) {
    currentSpectrum[i] = 0;
  }
  currentStyle = 0;
}

void setAudioStyle(uint8_t style) { currentStyle = style; }

void updateAudioSpectrum(const uint8_t *spectrum, uint8_t size) {
  uint8_t count = min(size, (uint8_t)16);
  for (int i = 0; i < count; i++) {
    // Clamp value to 16
    currentSpectrum[i] = min(spectrum[i], (uint8_t)16);
  }
}

// Helper to convert HSV to RGB
Pixel hsvToRgb(uint8_t h, uint8_t s, uint8_t v) {
  uint8_t r, g, b;
  uint8_t region, remainder, p, q, t;

  if (s == 0) {
    return Pixel{v, v, v};
  }

  region = h / 43;
  remainder = (h - (region * 43)) * 6;

  p = (v * (255 - s)) >> 8;
  q = (v * (255 - ((s * remainder) >> 8))) >> 8;
  t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

  switch (region) {
  case 0:
    r = v;
    g = t;
    b = p;
    break;
  case 1:
    r = q;
    g = v;
    b = p;
    break;
  case 2:
    r = p;
    g = v;
    b = t;
    break;
  case 3:
    r = p;
    g = q;
    b = v;
    break;
  case 4:
    r = t;
    g = p;
    b = v;
    break;
  default:
    r = v;
    g = p;
    b = q;
    break;
  }
  return Pixel{r, g, b};
}

void drawAudioBars() {
  for (int x = 0; x < 16; x++) {
    uint8_t height = currentSpectrum[x];

    // Calculate color based on X position (Rainbow effect)
    // Map 0-15 to 0-255 Hue
    uint8_t hue = (x * 255) / 16;
    Pixel color = hsvToRgb(hue, 255, 255);

    for (int y = 0; y < height; y++) {
      drawPixel(x, 15 - y, color);
    }
  }
}

void drawAudioRadial() {
  // Calculate average energy
  int total = 0;
  for (int i = 0; i < 16; i++) {
    total += currentSpectrum[i];
  }
  uint8_t avg = total / 16;

  // Map average (0-16) to radius (0-8)
  float radius = (float)avg * 8.0f / 16.0f;

  // Center at 7.5, 7.5 to be exactly in the middle of 16x16
  float cx = 7.5f;
  float cy = 7.5f;

  uint8_t hue = (millis() / 20) % 256;
  Pixel color = hsvToRgb(hue, 255, 255);
  Pixel dimColor = hsvToRgb(hue, 255, 50); // Faint glow

  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      float dx = (float)x - cx;
      float dy = (float)y - cy;
      float dist = sqrt(dx * dx + dy * dy);

      if (dist < radius) {
        // Inside the circle
        drawPixel(x, y, color);
      } else if (dist < radius + 1.0f) {
        // Edge smoothing / AA-like effect
        // Dim based on how far out it is
        uint8_t val = 255 - (uint8_t)((dist - radius) * 200);
        // Ensure we don't underflow
        if (val > 255)
          val = 0;
        // Simple dimmer: just use a fixed dim color for "edge" pixels or
        // compute
        Pixel aaColor = hsvToRgb(hue, 255, val);
        drawPixel(x, y, aaColor);
      } else if (dist < radius + 3.0f) {
        // Outer faint glow
        if ((x + y) % 2 == 0 && radius > 2.0f)
          drawPixel(x, y, dimColor);
      }
    }
  }
}

void drawAudioVisualizer() {
  // Clear display buffer but don't show yet
  clearBuffer();

  if (currentStyle == 1) {
    drawAudioRadial();
  } else {
    drawAudioBars();
  }

  showDisplay();
}
