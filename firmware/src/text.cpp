#include "text.h"
#include <string.h>

// 5x7 Font (Standard ASCII Subset: 0-9, A-Z, Space)
// Each char is 5 bytes (columns). LSB = top row.
static const uint8_t font5x7[][5] = {{0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
                                     {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
                                     {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
                                     {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
                                     {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
                                     {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
                                     {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
                                     {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
                                     {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
                                     {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9

                                     // A-Z (Offset 10)
                                     {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
                                     {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
                                     {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
                                     {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
                                     {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
                                     {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
                                     {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
                                     {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
                                     {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
                                     {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
                                     {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
                                     {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
                                     {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
                                     {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
                                     {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
                                     {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
                                     {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
                                     {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
                                     {0x46, 0x49, 0x49, 0x49, 0x31}, // S
                                     {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
                                     {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
                                     {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
                                     {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
                                     {0x63, 0x14, 0x08, 0x14, 0x63}, // X
                                     {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
                                     {0x61, 0x51, 0x49, 0x45, 0x43}, // Z

                                     // Space (Offset 36)
                                     {0x00, 0x00, 0x00, 0x00, 0x00}};

static char currentMessage[128] = "HELLO";
static Pixel currentColor = {0, 255, 0};
static uint8_t currentSpeed = 100; // ms per shift
static int scrollX = 0;
static unsigned long lastUpdate = 0;
static int messagePixelWidth = 0;

int getCharIndex(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'A' && c <= 'Z')
    return 10 + (c - 'A');
  if (c >= 'a' && c <= 'z')
    return 10 + (c - 'a'); // Handle lowercase as uppercase
  return 36;               // Space or unknown
}

void calculateWidth() {
  int len = strlen(currentMessage);
  messagePixelWidth = len * 6; // 5px width + 1px space
}

void initText() {
  scrollX = 16; // Start off-screen right
  calculateWidth();
}

void setText(const char *msg, Pixel color, uint8_t speed) {
  strncpy(currentMessage, msg, 127);
  currentMessage[127] = '\0';
  currentColor = color;
  currentSpeed = speed; // Ensure speed is reasonable
  if (currentSpeed < 20)
    currentSpeed = 20;

  scrollX = 16;
  calculateWidth();
  lastUpdate = 0; // FORCE IMMEDIATE UPDATE
}

void drawText() {
  // Clear text area (lines 4-10 approx) or whole screen safe
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      drawPixel(x, y, {0, 0, 0});
    }
  }

  int cursorX = scrollX;
  int yOffset = 4; // Center vertically (16 - 7) / 2 = 4.5 -> 4

  for (int i = 0; i < (int)strlen(currentMessage); i++) {
    int charIdx = getCharIndex(currentMessage[i]);

    // Draw char (5 columns)
    for (int col = 0; col < 5; col++) {
      int drawX = cursorX + col;
      if (drawX >= 0 && drawX < 16) {
        uint8_t columnByte = font5x7[charIdx][col];
        for (int row = 0; row < 7; row++) { // 7 pixels high
          if (columnByte & (1 << row)) {
            // LSB is top
            drawPixel(drawX, yOffset + row, currentColor);
          }
        }
      }
    }
    cursorX += 6; // 5 width + 1 space
  }
}

void updateText() {
  if (millis() - lastUpdate >= currentSpeed) {
    lastUpdate = millis();
    scrollX--;
    if (scrollX < -messagePixelWidth) {
      scrollX = 16; // Loop
    }
    drawText();
    showDisplay();
  }
}
