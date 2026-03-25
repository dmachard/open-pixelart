#include <Arduino.h>

#include "ble.h"
#include "clock.h"
#include "config.h"
#include "decoder.h"
#include "display.h"
#include "nightlight.h"
#include "storage.h"
#include "text.h"

#define MAX_FRAMES 15

#include "audio.h"
#include "game.h"

FrameDecoder decoder;
Frame decodedFrame;
Frame frames[MAX_FRAMES];

uint8_t frameCount = 0;
uint8_t currentFrameIndex = 0;
unsigned long lastFrameChange = 0;

uint8_t storedBrightness = 25;
uint8_t storedClockColorIndex = 2;    // Cyan by default
uint8_t storedClockGradientIndex = 0; // Rainbow by default
uint8_t defaultMode = MODE_CLOCK;
uint8_t currentMode = MODE_CLOCK;
bool nightLightActive = false; // true while the veilleuse is displayed
bool bleActive = true;
unsigned long lastBLEActivity = 0;
uint8_t bleTimeoutMinutes = 15;

volatile bool isUpdatingDisplay = false;

void updateDisplay() {
  if (isUpdatingDisplay)
    return;
  isUpdatingDisplay = true;

  if (currentMode == MODE_TEXT) {
    updateText();
    isUpdatingDisplay = false;
    return;
  }

  if (currentMode == MODE_CLOCK) {
    updateClock();
    isUpdatingDisplay = false;
    return;
  }

  if (currentMode == MODE_GALLERY && frameCount > 0) {
    unsigned long now = millis();
    Frame &f = frames[currentFrameIndex];

    if (now - lastFrameChange >= f.duration_ms) {
      currentFrameIndex = (currentFrameIndex + 1) % frameCount;
      lastFrameChange = now;
      displayFrame(frames[currentFrameIndex]);
    }
  }
  isUpdatingDisplay = false;
}

void onIncomingData(uint8_t *data, size_t len) {
  lastBLEActivity = millis();
  if (!decoder.decode(decodedFrame, data, len)) {
    Serial.println("Failed to decode frame");
    return;
  }

  if (isUpdatingDisplay)
    return;
  isUpdatingDisplay = true;

  switch (decodedFrame.deviceMode) {
  case MODE_TEXT:
    currentMode = MODE_TEXT;
    setText(decodedFrame.textMsg, decodedFrame.textColor,
            decodedFrame.textSpeed, decodedFrame.fontIndex);

    // Update brightness if needed
    if (decodedFrame.brightness != storedBrightness) {
      storedBrightness = decodedFrame.brightness;
      setDisplayBrightness(storedBrightness);
      saveBrightness(storedBrightness);
    }
    Serial.printf("Text Mode: %s (Speed: %d)\n", decodedFrame.textMsg,
                  decodedFrame.textSpeed);
    break;

  case MODE_AUDIO:
    currentMode = MODE_AUDIO;
    setAudioStyle(decodedFrame.audioStyle);
    updateAudioSpectrum(decodedFrame.spectrum, 16);
    drawAudioVisualizer();
    break;

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
    // REMOVED `saveFrame(frames[0]);` to prevent NVS flash wear and BLE packet
    // drops during fast drawing strokes (up to 50 frames/sec sent by webapp).
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
      // saveFrame(frames[0]); // REMOVED: Saving during animation load causes
      // BLE drops and first frame visual glitch
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
    Serial.printf("Clock update: Idx=%d, Total=%d\n", decodedFrame.frameIndex,
                  decodedFrame.frameTotal);

    // Check if a color index was provided in frameIndex
    if (decodedFrame.frameIndex < CLOCK_PALETTE_SIZE) {
      if (storedClockColorIndex != decodedFrame.frameIndex) {
        storedClockColorIndex = decodedFrame.frameIndex;
        currentClockColorIndex = storedClockColorIndex; // Sync BLE manager
        setClockColorIndex(storedClockColorIndex);
        saveClockColor(storedClockColorIndex);
        Serial.printf("Clock color saved to NVS: %d\n", storedClockColorIndex);
      }
    }

    // Check if a gradient index was provided in frameTotal (repurposed)
    // frameTotal is typically 0 or 1 for single frame modes, but we can usage
    // it here.
    if (decodedFrame.frameTotal < 4) { // We have 4 gradients (0-3)
      if (storedClockGradientIndex != decodedFrame.frameTotal) {
        storedClockGradientIndex = decodedFrame.frameTotal;
        currentClockGradientIndex =
            storedClockGradientIndex; // Sync BLE manager
        saveClockGradient(storedClockGradientIndex);
        setClockGradientIndex(storedClockGradientIndex);
        Serial.printf("Clock gradient saved to NVS: %d\n",
                      storedClockGradientIndex);
      }
    }

    setDisplayBrightness(storedBrightness);
    syncClockWithRTC();
    drawClock();
    showDisplay();
    showDisplay();
    break;
  case MODE_GAME:
    currentMode = MODE_GAME;
    drawGameFrame(decodedFrame.gameData);
    break;
  case MODE_NIGHTLIGHT: {
    // Save and apply the new night-light configuration
    NightLightConfig cfg;
    cfg.enabled =
        (decodedFrame.textColor.r & 0x01) != 0; // mapped to textColor.r
    cfg.colorIndex = decodedFrame.frameIndex;   // mapped to frameIndex
    cfg.brightness = decodedFrame.brightness;   // mapped to brightness
    cfg.startHour = decodedFrame.frameTotal;    // mapped to frameTotal
    cfg.startMinute = decodedFrame.audioStyle;  // mapped to audioStyle
    cfg.endHour = decodedFrame.textSpeed;       // mapped to textSpeed
    cfg.endMinute = decodedFrame.fontIndex;     // mapped to fontIndex
    setNightLightConfig(cfg);
    saveNightLightConfig(cfg);
    Serial.printf("NightLight config received: enabled=%d %02d:%02d->%02d:%02d "
                  "color=%d bright=%d\n",
                  cfg.enabled, cfg.startHour, cfg.startMinute, cfg.endHour,
                  cfg.endMinute, cfg.colorIndex, cfg.brightness);
    break;
  }
  default:
    Serial.printf("Unknown Mode received: %d\n", decodedFrame.deviceMode);
    frameCount = 0;
    break;
  }
  isUpdatingDisplay = false;
}

// remove the rest of the file

void onClientDisconnect() {
  // Clear slideshow state but keep the last frame on screen
  frameCount = 0;
  currentFrameIndex = 0;
  Serial.println("Client disconnected - keeping last frame displayed");
}

void setup() {
  // 1. COMMENCE INITIALIZATION (SERIAL + STORAGE)
  Serial.begin(115200);
  Serial.println("\n--- PixelBox Stable Startup ---");

  initStorage();
  storedBrightness = loadBrightness(25);
  if (storedBrightness > 120)
    storedBrightness = 25;

  defaultMode = loadDefaultMode(MODE_CLOCK);
  currentMode = defaultMode;
  storedClockColorIndex = loadClockColor(2);
  storedClockGradientIndex = loadClockGradient(0);
  setClockColorIndex(storedClockColorIndex);
  setClockGradientIndex(storedClockGradientIndex);

  // 2. INITIALIZE DISPLAY (BLACK)
  initDisplay(storedBrightness);
  delay(200); // Let power and bus settle

  // 3. INITIALIZE RTC AND OTHER MODULES
  initClock();
  initText();
  initAudio();
  initGame();

  NightLightConfig nlCfg;
  loadNightLightConfig(nlCfg);
  setNightLightConfig(nlCfg);
  initNightLight();

  // 4. PERFORM FIRST RENDER
  syncClockWithRTC();
  if (currentMode == MODE_CLOCK) {
    drawClock();
    showDisplay();
  } else {
    if (loadFrame(frames[0])) {
      frameCount = 1;
      currentFrameIndex = 0;
      displayFrame(frames[0]);
    } else {
      currentMode = MODE_CLOCK;
      drawClock();
      showDisplay();
    }
  }

  // 5. START COMMUNICATION
  bleTimeoutMinutes = loadBLETimeout(15);
  lastBLEActivity = millis();
  initBLE(onIncomingData, onClientDisconnect, storedBrightness,
          storedClockColorIndex, storedClockGradientIndex);
  Serial.println("--- Setup Complete ---");
}

void loop() {
  // ---- Night-light automatic schedule check (every ~30s) ----
  static unsigned long lastNLCheck = 0;
  if (millis() - lastNLCheck >= 30000UL) {
    lastNLCheck = millis();
    syncClockWithRTC(); // refresh h/m/s from RTC
    uint8_t curH = 0, curM = 0, curS = 0;
    getCurrentTime(curH, curM, curS);
    bool shouldBeActive = isNightLightTime(curH, curM);
    if (shouldBeActive && !nightLightActive) {
      nightLightActive = true;
      Serial.println("NightLight: activating");
      drawNightLight();
    } else if (!shouldBeActive && nightLightActive) {
      nightLightActive = false;
      Serial.println("NightLight: deactivating, restoring previous mode");
      // Restore previous mode display
      if (currentMode == MODE_CLOCK) {
        setDisplayBrightness(storedBrightness);
        drawClock();
        showDisplay();
      } else if (frameCount > 0) {
        displayFrame(frames[currentFrameIndex]);
      }
    } else if (shouldBeActive && nightLightActive) {
      // Refresh the glow periodically
      drawNightLight();
    }
  }

  // Continuously update display (only when veilleuse is not active)
  if (!nightLightActive) {
    updateDisplay();
  }

  // BLE Timeout Logic
  if (bleActive) {
    if (isBLEConnected()) {
      lastBLEActivity = millis(); // Reset timeout while connected
    } else {
      if (bleTimeoutMinutes > 0 &&
          (millis() - lastBLEActivity >
           (unsigned long)bleTimeoutMinutes * 60 * 1000)) {
        Serial.println("BLE Timeout reached - stopping BLE to save power");
        stopBLE();
        bleActive = false;
      }
    }
  }

  // Allow background tasks to run
  yield();
}
