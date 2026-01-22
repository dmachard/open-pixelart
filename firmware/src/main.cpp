#include <Arduino.h>

#include "ble.h"
#include "clock.h"
#include "config.h"
#include "decoder.h"
#include "display.h"
#include "storage.h"

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

uint8_t storedBrightness = 25;
uint8_t storedClockColorIndex = 2; // Cyan by default
uint8_t defaultMode = MODE_CLOCK;
uint8_t currentMode = MODE_CLOCK;

volatile bool isUpdatingDisplay = false;

void updateDisplay() {
  if (isUpdatingDisplay)
    return;
  isUpdatingDisplay = true;

  if (currentMode == MODE_CLOCK) {
    updateClock();
    isUpdatingDisplay = false;
    return;
  }

  if (frameCount == 0) {
    isUpdatingDisplay = false;
    return;
  }

  unsigned long now = millis();
  Frame &f = frames[currentFrameIndex];

  if (now - lastFrameChange >= f.duration_ms) {
    currentFrameIndex = (currentFrameIndex + 1) % frameCount;
    lastFrameChange = now;
    displayFrame(frames[currentFrameIndex]);
  }
  isUpdatingDisplay = false;
}

void onIncomingData(uint8_t *data, size_t len) {

  if (!decoder.decode(decodedFrame, data, len)) {
    Serial.println("Failed to decode frame");
    return;
  }

  if (isUpdatingDisplay)
    return;
  isUpdatingDisplay = true;

  switch (decodedFrame.deviceMode) {
  case MODE_DRAW:
    currentMode = MODE_DRAW;
    frameCount = 1;
    currentFrameIndex = 0;
    frames[0] = decodedFrame;

    lastFrameChange = millis();

    if (decodedFrame.brightness != storedBrightness) {
      storedBrightness = decodedFrame.brightness;
      saveBrightness(storedBrightness);
      currentBLEBrightness = storedBrightness;
      Serial.printf("Brightness saved to NVS: %d\n", storedBrightness);
    }

    displayFrame(frames[0]);
    saveFrame(frames[0]);
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
        saveBrightness(storedBrightness);
        currentBLEBrightness = storedBrightness;
        Serial.printf("Brightness saved to NVS: %d\n", storedBrightness);
      }

      displayFrame(frames[0]);
      saveFrame(frames[0]);
      Serial.printf("Switched to GALLERY Mode: %d frames ready.\n", frameCount);
    }
    break;
  case MODE_SETTINGS:
    // Update brightness if changed
    if (decodedFrame.brightness != storedBrightness) {
      storedBrightness = decodedFrame.brightness;
      saveBrightness(storedBrightness);
      currentBLEBrightness = storedBrightness;
      Serial.printf("Brightness saved to NVS (Settings): %d\n",
                    storedBrightness);

      setDisplayBrightness(storedBrightness);

      // If there's a frame being displayed, update its brightness and refresh
      if (frameCount > 0 && currentMode != MODE_CLOCK) {
        frames[currentFrameIndex].brightness = storedBrightness;
        displayFrame(frames[currentFrameIndex]);
      } else if (currentMode == MODE_CLOCK) {
        drawClock();
        showDisplay();
      }
    }

    // Update default mode if frameIndex is used as mode carrier
    if (decodedFrame.frameIndex != 255) { // Assuming 255 means "no change"
      defaultMode = decodedFrame.frameIndex;
      saveDefaultMode(defaultMode);
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
        saveClockColor(storedClockColorIndex);
        setClockColorIndex(storedClockColorIndex);
        Serial.printf("Clock color saved to NVS: %d\n", storedClockColorIndex);
      }
    }

    setDisplayBrightness(storedBrightness);
    syncClockWithRTC();
    drawClock();
    showDisplay();
    break;
  default:
    Serial.printf("Unknown Mode received: %d\n", decodedFrame.deviceMode);
    frameCount = 0;
    break;
  }
  isUpdatingDisplay = false;
}

void onClientDisconnect() {
  // Clear slideshow state but keep the last frame on screen
  frameCount = 0;
  currentFrameIndex = 0;
  Serial.println("Client disconnected - keeping last frame displayed");
}

void setup() {
 // Initialize serial for debugging
  Serial.begin(115200);

  // Initialize clock
  initClock();

  // Initialize storage
  initStorage();
  storedBrightness = loadBrightness(25);
  storedClockColorIndex = loadClockColor(2);
  currentClockColorIndex = storedClockColorIndex; // Sync BLE manager
  setClockColorIndex(storedClockColorIndex);
  defaultMode = loadDefaultMode(MODE_CLOCK);
  currentMode = defaultMode;
  Serial.printf("Loaded brightness from NVS: %d\n", storedBrightness);
  Serial.printf("Loaded clock color from NVS: %d\n", storedClockColorIndex);
  Serial.printf("Loaded default mode from NVS: %d\n", defaultMode);

  // Initialize display
  initDisplay(storedBrightness);

  // Restore last image if available and not in clock mode
  if (currentMode != MODE_CLOCK && loadFrame(frames[0])) {
    frameCount = 1;
    currentFrameIndex = 0;
    displayFrame(frames[0]);
  } else if (currentMode == MODE_CLOCK) {
    drawClock();
    showDisplay();
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
