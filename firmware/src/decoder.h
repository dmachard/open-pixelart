#ifndef DECODER_H
#define DECODER_H

#include <Arduino.h>
#include "config.h"

#define FRAMEDEC_HEADER_SIZE 8
#define FRAMEDEC_MAX_PALETTE 16
#define FRAMEDEC_BYTES_PER_ROW (MATRIX_WIDTH / 2)

// Buffer size calculation:
// 16×16: 8 (header) + 48 (palette) + (16×16÷2) = 184 bytes
// 64×64: 8 (header) + 48 (palette) + (64×64÷2) = 2104 bytes
#define MAX_FRAME_BUFFER_SIZE (FRAMEDEC_HEADER_SIZE + (16 * 3) + (MATRIX_WIDTH * MATRIX_HEIGHT / 2) + 64)

struct Pixel {
    uint8_t r, g, b;
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
private:
    uint8_t frameBuffer[MAX_FRAME_BUFFER_SIZE];
    size_t currentSize = 0;
    uint8_t expectedFragments = 0;
    uint8_t receivedFragments = 0;
    uint32_t lastFragmentTime = 0;
    
    bool isFragmented(const uint8_t* data, size_t len);
    bool handleFragment(const uint8_t* data, size_t len);
    
public:
    bool decode(Frame &out, const uint8_t *data, size_t len);
    void checkTimeout();
    uint32_t totalFrames = 0;
    uint32_t droppedFrames = 0;
};

#endif