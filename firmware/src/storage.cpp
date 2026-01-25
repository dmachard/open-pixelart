#include "storage.h"
#include <Arduino.h>
#include <Preferences.h>
#include <stdint.h>

static Preferences preferences;

void initStorage() { preferences.begin("pixelart", false); }

void saveBrightness(uint8_t brightness) {
  preferences.putUChar("brightness", brightness);
}

uint8_t loadBrightness(uint8_t defaultVal) {
  return preferences.getUChar("brightness", defaultVal);
}

void saveClockColor(uint8_t colorIndex) {
  preferences.putUChar("clock_color", colorIndex);
}

uint8_t loadClockColor(uint8_t defaultVal) {
  return preferences.getUChar("clock_color", defaultVal);
}

void saveClockGradient(uint8_t gradientIndex) {
  preferences.putUChar("clock_grad", gradientIndex);
}

uint8_t loadClockGradient(uint8_t defaultVal) {
  return preferences.getUChar("clock_grad", defaultVal);
}

void saveDefaultMode(uint8_t mode) {
  preferences.putUChar("default_mode", mode);
}

uint8_t loadDefaultMode(uint8_t defaultVal) {
  return preferences.getUChar("default_mode", defaultVal);
}

void saveFrame(const Frame &f) {
  preferences.putBytes("last_frame", &f, sizeof(Frame));
  Serial.println("Frame saved to NVS");
}

bool loadFrame(Frame &f) {
  if (preferences.isKey("last_frame")) {
    size_t len = preferences.getBytes("last_frame", &f, sizeof(Frame));
    if (len == sizeof(Frame)) {
      Serial.println("Frame restored from NVS");
      return true;
    }
  }
  return false;
}
