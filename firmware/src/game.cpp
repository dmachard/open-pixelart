#include "game.h"

// Hardcoded Tetris palette to save bandwidth
// 0: Empty (Black)
// 1: Cyan (I)
// 2: Blue (J)
// 3: Orange (L)
// 4: Yellow (O)
// 5: Green (S)
// 6: Purple (T)
// 7: Red (Z)
// 8: Grey (Wall/Ghost)
static const Pixel gamePalette[] = {
    Pixel{0, 0, 0},       // 0: Black
    Pixel{0, 255, 255},   // 1: Cyan
    Pixel{0, 0, 255},     // 2: Blue
    Pixel{255, 165, 0},   // 3: Orange
    Pixel{255, 255, 0},   // 4: Yellow
    Pixel{0, 255, 0},     // 5: Green
    Pixel{128, 0, 128},   // 6: Purple
    Pixel{255, 0, 0},     // 7: Red
    Pixel{50, 50, 50},    // 8: Dark Grey
    Pixel{255, 255, 255}, // 9: White (Flash/Effect)
    Pixel{100, 100, 100}, // 10: Lighter Grey
    Pixel{0, 0, 0},       // 11-15 reserved
    Pixel{0, 0, 0},       Pixel{0, 0, 0}, Pixel{0, 0, 0}, Pixel{0, 0, 0}};

void initGame() {
  // Nothing specific to init for now
}

void drawGameFrame(const uint8_t *packedData) {
  // Clear buffer (implicit via rewrite, but good practice if partial updates)
  // Actually we will overwrite every pixel, so clear is optional but safe
  // clearBuffer();

  // packedData header is stripped by decoder/main, so we get 128 bytes
  // Each byte contains 2 pixels (4 bits each)

  for (int i = 0; i < 128; i++) {
    uint8_t byte = packedData[i];
    uint8_t idx1 = (byte >> 4) & 0x0F;
    uint8_t idx2 = byte & 0x0F;

    // Calculate X, Y
    // i * 2 = pixel index
    int p1_idx = i * 2;
    int p2_idx = i * 2 + 1;

    int y1 = p1_idx / 16;
    int x1 = p1_idx % 16;

    int y2 = p2_idx / 16;
    int x2 = p2_idx % 16;

    drawPixel(x1, y1, gamePalette[idx1]);
    drawPixel(x2, y2, gamePalette[idx2]);
  }

  showDisplay();
}
