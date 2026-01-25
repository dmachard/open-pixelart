#ifndef GAME_H
#define GAME_H

#include "display.h"
#include <Arduino.h>

// Define standard Tetris colors for efficient palette usage
// This allows us to send only 4-bit pixel indices
void initGame();
void drawGameFrame(const uint8_t *packedData);

#endif
