#ifndef FRAME_DECODER_H
#define FRAME_DECODER_H

#include <Arduino.h>

#include "config.h"

#define FRAMEDEC_HEADER_SIZE 8
#define FRAMEDEC_MAX_PALETTE 16
#define FRAMEDEC_BYTES_PER_ROW (MATRIX_WIDTH / 2)

struct Pixel {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct Frame {
    Pixel pixels[MATRIX_HEIGHT][MATRIX_WIDTH];
    uint16_t duration_ms;
    uint8_t brightness;
    uint8_t deviceMode;
    uint8_t frameIndex;
    uint8_t frameTotal;
};

class FrameDecoder {
public:
    static bool decode(Frame &out, const uint8_t *data, size_t len);
};

#endif