#pragma once
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
    static bool decode(Frame &out, const uint8_t *data, size_t len) {
        if (len < FRAMEDEC_HEADER_SIZE) return false;

        uint8_t deviceMode = data[0];
        uint8_t brightness = data[1];
        uint8_t paletteSize = min(data[2], (uint8_t)FRAMEDEC_MAX_PALETTE);
        uint8_t frameIndex = data[3];
        uint8_t frameTotal = data[4];
        uint16_t duration = data[6] | (data[7] << 8);
        if (duration < 1) duration = 1;

        size_t paletteBytes = paletteSize * 3;
        if (len < FRAMEDEC_HEADER_SIZE + paletteBytes) return false;

        Pixel palette[FRAMEDEC_MAX_PALETTE];
        for (uint8_t i = 0; i < paletteSize; i++) {
            int o = FRAMEDEC_HEADER_SIZE + i * 3;
            palette[i] = { data[o], data[o+1], data[o+2] };
        }

        out.deviceMode = deviceMode;
        out.frameIndex = frameIndex;
        out.frameTotal = frameTotal;
        out.brightness = brightness;
        out.duration_ms = duration * 1000UL;

        size_t pixelStart = FRAMEDEC_HEADER_SIZE + paletteBytes;
        for (int y = 0; y < MATRIX_HEIGHT; y++) {
            for (int x = 0; x < MATRIX_WIDTH; x += 2) {
                size_t byteIndex = pixelStart + (y * FRAMEDEC_BYTES_PER_ROW + x / 2);
                if (byteIndex >= len) return false;

                uint8_t pix = data[byteIndex];
                uint8_t idx1 = pix >> 4;
                uint8_t idx2 = pix & 0x0F;

                out.pixels[y][x] = (idx1 < paletteSize) ? palette[idx1] : Pixel{0,0,0};
                out.pixels[y][x + 1] = (idx2 < paletteSize) ? palette[idx2] : Pixel{0,0,0};
            }
        }

        return true;
    }
};
