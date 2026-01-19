#include <Arduino.h>

#include "ble.h"
#include "config.h"
#include "decoder.h"
#include "display.h"
#include <Preferences.h>

#define MAX_FRAMES 15
#define MODE_DRAW 0
#define MODE_GALLERY 1
#define MODE_SETTINGS 2
#define MODE_CLOCK 3

FrameDecoder decoder;
Frame decodedFrame;
Frame frames[MAX_FRAMES];

uint8_t frameCount = 0;
uint8_t currentFrameIndex = 0;
unsigned long lastFrameChange = 0;

Preferences preferences;
uint8_t storedBrightness = 25;
uint8_t storedClockColorIndex = 2; // Cyan by default
uint8_t defaultMode = MODE_CLOCK;
uint8_t currentMode = MODE_CLOCK;

// Clock color palette: Lime (Default), White, Cyan, Orange
const Pixel clockPalette[] = {
    Pixel{0, 255, 0},     // 0: Lime
    Pixel{255, 255, 255}, // 1: White
    Pixel{0, 255, 255},   // 2: Cyan
    Pixel{255, 165, 0}    // 3: Orange
};
const uint8_t CLOCK_PALETTE_SIZE = sizeof(clockPalette) / sizeof(Pixel);

// Digit patterns 3x5
const uint8_t digits[10][5] = {
    {0x07, 0x05, 0x05, 0x05, 0x07}, // 0
    {0x01, 0x01, 0x01, 0x01, 0x01}, // 1
    {0x07, 0x01, 0x07, 0x04, 0x07}, // 2
    {0x07, 0x01, 0x07, 0x01, 0x07}, // 3
    {0x05, 0x05, 0x07, 0x01, 0x01}, // 4
    {0x07, 0x04, 0x07, 0x01, 0x07}, // 5
    {0x07, 0x04, 0x07, 0x05, 0x07}, // 6
    {0x07, 0x01, 0x01, 0x01, 0x01}, // 7
    {0x07, 0x05, 0x07, 0x05, 0x07}, // 8
    {0x07, 0x05, 0x07, 0x01, 0x07}  // 9
};

struct ClockManager {
  uint8_t hours = 19;
  uint8_t minutes = 53;
  uint8_t seconds = 0;
  unsigned long lastTick = 0;

  void update() {
    if (millis() - lastTick >= 1000) {
      lastTick = millis();
      seconds++;
      if (seconds >= 60) {
        seconds = 0;
        minutes++;
        if (minutes >= 60) {
          minutes = 0;
          hours = (hours + 1) % 24;
        }
      }
      draw();
      show();
    }
  }

  void drawDigit(int x, int y, int digit, Pixel color) {
    if (digit < 0 || digit > 9)
      return;
    for (int row = 0; row < 5; row++) {
      for (int col = 0; col < 3; col++) {
        if (digits[digit][row] & (1 << (2 - col))) {
          drawPixel(x + col, y + row, color);
        }
      }
    }
  }

  void draw() {
    // Clear border
    for (int x = 0; x < 16; x++) {
      drawPixel(x, 0, Pixel{0, 0, 0});
      drawPixel(x, 15, Pixel{0, 0, 0});
    }
    for (int y = 1; y < 15; y++) {
      drawPixel(0, y, Pixel{0, 0, 0});
      drawPixel(15, y, Pixel{0, 0, 0});
    }

    // Draw animated border (60 pixels)
    for (int s = 0; s <= seconds; s++) {
      int bx, by;
      if (s < 16) {
        bx = s;
        by = 0;
      } else if (s < 30) {
        bx = 15;
        by = s - 15;
      } else if (s < 46) {
        bx = 15 - (s - 30);
        by = 15;
      } else {
        bx = 0;
        by = 15 - (s - 45);
      }

      Pixel color;
      if (s <= 7 || s >= 53)
        color = Pixel{255, 0, 0}; // Red
      else if (s <= 22)
        color = Pixel{255, 255, 0}; // Yellow
      else if (s <= 37)
        color = Pixel{0, 255, 0}; // Green
      else
        color = Pixel{0, 0, 255}; // Blue
      drawPixel(bx, by, color);
    }

    // Clear background (inner)
    for (int y = 1; y < 15; y++) {
      for (int x = 1; x < 15; x++) {
        drawPixel(x, y, Pixel{0, 0, 0});
      }
    }

    Pixel color = clockPalette[min(storedClockColorIndex,
                                   (uint8_t)(CLOCK_PALETTE_SIZE - 1))];
    // HH (3px digits, 1px space, shifted 1px left to x=6)
    drawDigit(6, 2, hours / 10, color);
    drawDigit(10, 2, hours % 10, color);
    // MM
    drawDigit(6, 9, minutes / 10, color);
    drawDigit(10, 9, minutes % 10, color);
    // Separator (colon) shifted 1px left to x=4
    if (seconds % 2 == 0) {
      drawPixel(4, 7, color);
      drawPixel(4, 9, color);
    }
  }

  void show() { showDisplay(); }
};

ClockManager clockMgr;

void saveFrameToNVS(const Frame &f) {
  preferences.putBytes("last_frame", &f, sizeof(Frame));
  Serial.println("💾 Frame saved to NVS");
}

bool loadFrameFromNVS(Frame &f) {
  if (preferences.isKey("last_frame")) {
    size_t len = preferences.getBytes("last_frame", &f, sizeof(Frame));
    if (len == sizeof(Frame)) {
      Serial.println("📂 Frame restored from NVS");
      return true;
    }
  }
  return false;
}

void updateDisplay() {
  if (currentMode == MODE_CLOCK) {
    clockMgr.update();
    return;
  }

  if (frameCount == 0)
    return;

  unsigned long now = millis();
  Frame &f = frames[currentFrameIndex];

  if (now - lastFrameChange >= f.duration_ms) {
    currentFrameIndex = (currentFrameIndex + 1) % frameCount;
    lastFrameChange = now;
    displayFrame(frames[currentFrameIndex]);
  }
}

void onIncomingData(uint8_t *data, size_t len) {

  if (!decoder.decode(decodedFrame, data, len)) {
    Serial.println("Failed to decode frame");
    return;
  }

  switch (decodedFrame.deviceMode) {
  case MODE_DRAW:
    currentMode = MODE_DRAW;
    frameCount = 1;
    currentFrameIndex = 0;
    frames[0] = decodedFrame;

    lastFrameChange = millis();

    if (decodedFrame.brightness != storedBrightness) {
      storedBrightness = decodedFrame.brightness;
      preferences.putUChar("brightness", storedBrightness);
      currentBLEBrightness = storedBrightness;
      Serial.printf("Brightness saved to NVS: %d\n", storedBrightness);
    }

    displayFrame(frames[0]);
    saveFrameToNVS(frames[0]);
    Serial.println("Switched to DRAW Mode.");
    break;
  case MODE_GALLERY:
    currentMode = MODE_GALLERY;
    // On first frame, set total frame count
    if (decodedFrame.frameIndex == 0) {
      frameCount = min((uint8_t)decodedFrame.frameTotal, (uint8_t)MAX_FRAMES);
      currentFrameIndex = 0;
    }

    // Store the received frame at its index
    if (decodedFrame.frameIndex < frameCount) {
      frames[decodedFrame.frameIndex] = decodedFrame;
    }

    // Start slideshow if last frame received
    if (decodedFrame.frameIndex + 1 == decodedFrame.frameTotal &&
        frameCount > 0) {
      lastFrameChange = millis();

      if (decodedFrame.brightness != storedBrightness) {
        storedBrightness = decodedFrame.brightness;
        preferences.putUChar("brightness", storedBrightness);
        currentBLEBrightness = storedBrightness;
        Serial.printf("Brightness saved to NVS: %d\n", storedBrightness);
      }

      displayFrame(frames[0]);
      saveFrameToNVS(frames[0]);
      Serial.printf("Switched to GALLERY Mode: %d frames ready.\n", frameCount);
    }
    break;
  case MODE_SETTINGS:
    // Update brightness if changed
    if (decodedFrame.brightness != storedBrightness) {
      storedBrightness = decodedFrame.brightness;
      preferences.putUChar("brightness", storedBrightness);
      currentBLEBrightness = storedBrightness;
      Serial.printf("Brightness saved to NVS (Settings): %d\n",
                    storedBrightness);

      setDisplayBrightness(storedBrightness);

      // If there's a frame being displayed, update its brightness and refresh
      if (frameCount > 0 && currentMode != MODE_CLOCK) {
        frames[currentFrameIndex].brightness = storedBrightness;
        displayFrame(frames[currentFrameIndex]);
      } else if (currentMode == MODE_CLOCK) {
        clockMgr.draw();
        clockMgr.show();
      }
    }

    // Update default mode if frameIndex is used as mode carrier
    if (decodedFrame.frameIndex != 255) { // Assuming 255 means "no change"
      defaultMode = decodedFrame.frameIndex;
      preferences.putUChar("default_mode", defaultMode);
      Serial.printf("Default mode saved to NVS: %d\n", defaultMode);
    }
    break;
  case MODE_CLOCK:
    currentMode = MODE_CLOCK;
    Serial.println("Switched to CLOCK Mode.");

    // Check if a color index was provided in frameIndex
    if (decodedFrame.frameIndex < CLOCK_PALETTE_SIZE) {
      if (storedClockColorIndex != decodedFrame.frameIndex) {
        storedClockColorIndex = decodedFrame.frameIndex;
        currentClockColorIndex = storedClockColorIndex; // Sync BLE manager
        preferences.putUChar("clock_color", storedClockColorIndex);
        Serial.printf("Clock color saved to NVS: %d\n", storedClockColorIndex);
      }
    }

    setDisplayBrightness(storedBrightness);
    clockMgr.draw();
    clockMgr.show();
    break;
  default:
    Serial.printf("Unknown Mode received: %d\n", decodedFrame.deviceMode);
    frameCount = 0;
    break;
  }
}

void onClientDisconnect() {
  // Clear slideshow state but keep the last frame on screen
  frameCount = 0;
  currentFrameIndex = 0;
  Serial.println("Client disconnected - keeping last frame displayed");
}

void setup() {
  // Start Serial for debugging
  Serial.begin(115200);

  // Initialize storage
  preferences.begin("pixelart", false);
  storedBrightness = preferences.getUChar("brightness", 25);
  storedClockColorIndex = preferences.getUChar("clock_color", 2);
  currentClockColorIndex = storedClockColorIndex; // Sync BLE manager
  defaultMode = preferences.getUChar("default_mode", MODE_CLOCK);
  currentMode = defaultMode;
  Serial.printf("Loaded brightness from NVS: %d\n", storedBrightness);
  Serial.printf("Loaded clock color from NVS: %d\n", storedClockColorIndex);
  Serial.printf("Loaded default mode from NVS: %d\n", defaultMode);

  // Initialize display
  initDisplay(storedBrightness);

  // Restore last image if available and not in clock mode
  if (currentMode != MODE_CLOCK && loadFrameFromNVS(frames[0])) {
    frameCount = 1;
    currentFrameIndex = 0;
    displayFrame(frames[0]);
  } else if (currentMode == MODE_CLOCK) {
    clockMgr.draw();
    clockMgr.show();
  }

  // Initialize BLE with callbacks
  initBLE(onIncomingData, onClientDisconnect, storedBrightness,
          storedClockColorIndex);
}

void loop() {
  // Continuously update display
  updateDisplay();

  // Allow background tasks to run
  yield();
}
