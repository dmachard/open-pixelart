#include "nightlight.h"
#include "display.h"

static NightLightConfig nlConfig = {
    false,    // enabled
    22,    0, // startHour, startMinute
    7,     0, // endHour, endMinute
    0,        // colorIndex (orange chaud)
    5         // brightness
};

void initNightLight() {
  // Nothing to initialise hardware-wise; config is loaded by storage.
}

void setNightLightConfig(const NightLightConfig &cfg) { nlConfig = cfg; }

const NightLightConfig &getNightLightConfig() { return nlConfig; }

// Returns true if (hour:minute) is inside the night window.
// Handles cross-midnight schedules (e.g. 22:00 → 06:00).
bool isNightLightTime(uint8_t hour, uint8_t minute) {
  if (!nlConfig.enabled)
    return false;

  uint16_t now = (uint16_t)hour * 60 + minute;
  uint16_t start = (uint16_t)nlConfig.startHour * 60 + nlConfig.startMinute;
  uint16_t end = (uint16_t)nlConfig.endHour * 60 + nlConfig.endMinute;

  if (start <= end) {
    // Same day window (e.g. 08:00 → 12:00)
    bool active = now >= start && now < end;
    Serial.printf("NL check (same day): now=%d start=%d end=%d active=%d\n",
                  now, start, end, active);
    return active;
  } else {
    // Cross-midnight window (e.g. 22:00 → 06:00)
    bool active = now >= start || now < end;
    Serial.printf("NL check (cross night): now=%d start=%d end=%d active=%d\n",
                  now, start, end, active);
    return active;
  }
}

void drawNightLight() {
  // Turn off the display completely (all black)
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      drawPixel(x, y, Pixel{0, 0, 0});
    }
  }

  setDisplayBrightness(0);
  showDisplay();
}
