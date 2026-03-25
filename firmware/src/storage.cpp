#include "storage.h"
#include "nightlight.h"
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

void saveBLETimeout(uint8_t minutes) {
  preferences.putUChar("ble_timeout", minutes);
}

uint8_t loadBLETimeout(uint8_t defaultVal) {
  return preferences.getUChar("ble_timeout", defaultVal);
}

void saveNightLightConfig(const NightLightConfig &cfg) {
  preferences.putBool("nl_enabled", cfg.enabled);
  preferences.putUChar("nl_start_h", cfg.startHour);
  preferences.putUChar("nl_start_m", cfg.startMinute);
  preferences.putUChar("nl_end_h", cfg.endHour);
  preferences.putUChar("nl_end_m", cfg.endMinute);
  preferences.putUChar("nl_color", cfg.colorIndex);
  preferences.putUChar("nl_bright", cfg.brightness);
  Serial.println("NightLight config saved to NVS");
}

void loadNightLightConfig(NightLightConfig &cfg) {
  cfg.enabled = preferences.getBool("nl_enabled", false);
  cfg.startHour = preferences.getUChar("nl_start_h", 22);
  cfg.startMinute = preferences.getUChar("nl_start_m", 0);
  cfg.endHour = preferences.getUChar("nl_end_h", 7);
  cfg.endMinute = preferences.getUChar("nl_end_m", 0);
  cfg.colorIndex = preferences.getUChar("nl_color", 0);
  cfg.brightness = preferences.getUChar("nl_bright", 5);
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
