#include "clock.h"
#include "storage.h"
#include <RTClib.h>
#include <Wire.h>

// Digit patterns 3x5
static const uint8_t digits[10][5] = {
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

// Clock color palette: Lime (Default), White, Cyan, Orange
static const Pixel clockPalette[] = {
    Pixel{0, 255, 0},     // 0: Lime
    Pixel{255, 255, 255}, // 1: White
    Pixel{0, 255, 255},   // 2: Cyan
    Pixel{255, 165, 0}    // 3: Orange
};
const uint8_t CLOCK_PALETTE_SIZE = sizeof(clockPalette) / sizeof(Pixel);

static RTC_DS3231 rtc;
static uint8_t hours = 0;
static uint8_t minutes = 0;
static uint8_t seconds = 0;
static uint8_t storedColorIndex = 2;
static uint8_t storedGradientIndex = 0;
static unsigned long lastTick = 0;
static unsigned long lastSync = 0;
static bool needsSync = true;

static void drawDigit(int x, int y, int digit, Pixel color) {
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

void initClock() {
  Serial.println("Initializing RTC...");
  Wire.begin(RTC_SDA_PIN, RTC_SCL_PIN);
  Wire.setClock(400000);

  if (!rtc.begin()) {
    Serial.println("couldn't find RTC!");
  } else {
    Serial.println("RTC started successfully.");
    if (rtc.lostPower()) {
      Serial.println("RTC lost power, let's set the time!");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    syncClockWithRTC();
  }
}

// Helper to determine if a date is in European Summer Time (CEST)
// Rules: Starts last Sunday of March 02:00 CET, Ends last Sunday of October
// 03:00 CEST.
bool isEuropeanSummerTime(DateTime dt) {
  int month = dt.month();
  int day = dt.day();
  int hour = dt.hour();
  int dow = dt.dayOfTheWeek(); // 0 is Sunday

  if (month < 3 || month > 10)
    return false;
  if (month > 3 && month < 10)
    return true;

  // Last Sunday of March or October
  // A day is in the "last week" if d + (7 - dow) > 31
  if (month == 3) {
    if (day - dow < 25)
      return false;
    if (dow == 0 && hour < 2)
      return false;
    return true;
  } else if (month == 10) {
    if (day - dow < 25)
      return true;
    if (dow == 0 && hour < 3)
      return true;
    return false;
  }
  return false;
}

void syncClockWithRTC() {
  DateTime now = rtc.now();
  hours = now.hour();

  if (loadAutoDST(true)) {
    if (isEuropeanSummerTime(now)) {
      hours = (hours + 1) % 24;
    }
  }

  minutes = now.minute();
  seconds = now.second();
  lastSync = millis();
  lastTick = millis(); // Reset animation timer to prevent "catch-up" effect
  needsSync = false;
  Serial.printf("RTC Synced: %02d:%02d:%02d\n", hours, minutes, seconds);
}

void getCurrentTime(uint8_t &h, uint8_t &m, uint8_t &s) {
  h = hours;
  m = minutes;
  s = seconds;
}

void setClockColorIndex(uint8_t index) {
  if (index < CLOCK_PALETTE_SIZE) {
    storedColorIndex = index;
  }
}

void setClockGradientIndex(uint8_t index) { storedGradientIndex = index; }

void drawClock() {
  // Sanitize values to prevent out-of-bounds rendering
  uint8_t h = hours % 24;
  uint8_t m = minutes % 60;
  uint8_t s = seconds % 60;

  // 1. Clear entire screen first (0-15)
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      drawPixel(x, y, Pixel{0, 0, 0});
    }
  }

  // 2. Draw animated border
  for (int i = 0; i <= s; i++) {
    int pos = (i + 8) % 60;
    int bx, by;
    if (pos < 16) {
      bx = pos;
      by = 0;
    } else if (pos < 30) {
      bx = 15;
      by = pos - 15;
    } else if (pos < 46) {
      bx = 15 - (pos - 30);
      by = 15;
    } else {
      bx = 0;
      by = 15 - (pos - 45);
    }

    Pixel borderCol;
    if (storedGradientIndex == 0) {
      // Classic Rainbow
      if (i < 15)
        borderCol = Pixel{255, 255, 0};
      else if (i < 30)
        borderCol = Pixel{0, 255, 0};
      else if (i < 45)
        borderCol = Pixel{0, 0, 255};
      else
        borderCol = Pixel{255, 0, 0};
    } else if (storedGradientIndex == 1) {
      borderCol = Pixel{255, 255, 255};
    } else if (storedGradientIndex == 2) {
      if (i < 20)
        borderCol = Pixel{0, 0, 255};
      else if (i < 40)
        borderCol = Pixel{0, 255, 255};
      else
        borderCol = Pixel{128, 0, 255};
    } else if (storedGradientIndex == 3) {
      if (i < 20)
        borderCol = Pixel{255, 0, 0};
      else if (i < 40)
        borderCol = Pixel{255, 165, 0};
      else
        borderCol = Pixel{255, 255, 0};
    } else {
      borderCol = Pixel{255, 255, 255};
    }

    drawPixel(bx, by, borderCol);
  }

  // 3. Draw digits and separator
  Pixel color =
      clockPalette[min(storedColorIndex, (uint8_t)(CLOCK_PALETTE_SIZE - 1))];
  drawDigit(6, 2, h / 10, color);
  drawDigit(10, 2, h % 10, color);
  drawDigit(6, 9, m / 10, color);
  drawDigit(10, 9, m % 10, color);

  if (s % 2 == 0) {
    drawPixel(4, 7, color);
    drawPixel(4, 9, color);
  }
}

void updateClock() {
  if (millis() - lastTick >= 1000) {
    lastTick += 1000;

    if (needsSync || (millis() - lastSync >= 60000)) {
      syncClockWithRTC();
    } else {
      seconds++;
      if (seconds >= 60) {
        seconds = 0;
        minutes++;
        if (minutes >= 60) {
          minutes = 0;
          hours = (hours + 1) % 24;
        }
      }
    }
    drawClock();
    showDisplay();
  }
}
