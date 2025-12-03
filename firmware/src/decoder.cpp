#include "decoder.h"

bool FrameDecoder::decode(Frame &out, const uint8_t *data, size_t len) {
    // check minimum length
    if (len < FRAMEDEC_HEADER_SIZE) return false;

    uint8_t deviceMode = data[0];
    uint8_t brightness = data[1];
    uint8_t paletteSize = min(data[2], (uint8_t)FRAMEDEC_MAX_PALETTE);
    uint8_t frameIndex = data[3];
    uint8_t frameTotal = data[4];
    uint16_t duration = data[6] | (data[7] << 8);
    if (duration < 1) duration = 1;

    // read palette
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
    
    // loop on rows
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
        // loop on columns, 2 pixels per byte
        for (int x = 0; x < MATRIX_WIDTH; x += 2) {
            
            // compute byte index in data array
            size_t byteIndex = pixelStart + (y * FRAMEDEC_BYTES_PER_ROW + x / 2);
            if (byteIndex >= len) return false;

            uint8_t pix = data[byteIndex];
            
            // same as: uint8_t idx1 = (pix >> 4) & 0x0F;
            uint8_t idx1 = pix >> 4;
            uint8_t idx2 = pix & 0x0F;

            // assign decoded pixels to output frame
            out.pixels[y][x] = (idx1 < paletteSize) ? palette[idx1] : Pixel{0,0,0};
            out.pixels[y][x + 1] = (idx2 < paletteSize) ? palette[idx2] : Pixel{0,0,0};
        }
    }

    return true;
}