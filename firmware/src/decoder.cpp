#include "decoder.h"

#define FRAGMENT_TIMEOUT_MS 500

bool FrameDecoder::isFragmented(const uint8_t* data, size_t len) {
    return len > 0 && data[0] == 0xFF;
}

bool FrameDecoder::handleFragment(const uint8_t* data, size_t len) {
    if (len < 3) {
        Serial.println("❌ Fragment too small");
        droppedFrames++;
        return false;
    }
    
    uint8_t fragIndex = data[1];
    uint8_t fragTotal = data[2];
    
    // Premier fragment: initialiser
    if (fragIndex == 0) {
        currentSize = 0;
        expectedFragments = fragTotal;
        receivedFragments = 0;
        lastFragmentTime = millis();
        Serial.printf("📦 Frame start: %d fragments expected\n", fragTotal);
    }
    
    // Vérifier l'ordre des fragments
    if (fragIndex != receivedFragments) {
        Serial.printf("❌ Fragment out of order! Expected %d, got %d\n", 
                      receivedFragments, fragIndex);
        droppedFrames++;
        currentSize = 0;
        receivedFragments = 0;
        return false;
    }
    
    // Vérifier cohérence du nombre total
    if (fragTotal != expectedFragments) {
        Serial.println("❌ Fragment count mismatch");
        droppedFrames++;
        currentSize = 0;
        receivedFragments = 0;
        return false;
    }
    
    // Copier les données (skip les 3 premiers bytes: 0xFF, index, total)
    size_t dataSize = len - 3;
    if (currentSize + dataSize > MAX_FRAME_BUFFER_SIZE) {
        Serial.printf("❌ Buffer overflow: %d + %d > %d\n", 
                      currentSize, dataSize, MAX_FRAME_BUFFER_SIZE);
        droppedFrames++;
        currentSize = 0;
        receivedFragments = 0;
        return false;
    }
    
    memcpy(frameBuffer + currentSize, data + 3, dataSize);
    currentSize += dataSize;
    receivedFragments++;
    lastFragmentTime = millis();
    
    Serial.printf("✓ Fragment %d/%d (%d bytes, total: %d)\n", 
                  fragIndex + 1, fragTotal, dataSize, currentSize);
    
    // Frame complète?
    if (receivedFragments == expectedFragments) {
        totalFrames++;
        Serial.printf("✅ Frame complete! (Success rate: %.1f%%)\n", 
                      100.0 * totalFrames / (totalFrames + droppedFrames));
        return true;
    }
    
    return false;
}

void FrameDecoder::checkTimeout() {
    // if we are in the middle of receiving fragments, check for timeout
    if (receivedFragments > 0 && receivedFragments < expectedFragments) {
        if (millis() - lastFragmentTime > FRAGMENT_TIMEOUT_MS) {
            Serial.printf("⏱️ Fragment timeout! Lost %d/%d fragments\n", 
                          expectedFragments - receivedFragments, expectedFragments);
            droppedFrames++;
            currentSize = 0;
            receivedFragments = 0;
        }
    }
}

bool FrameDecoder::decode(Frame &out, const uint8_t *data, size_t len) {
    Serial.printf("Decoding frame: %d bytes\n", len);
    // fragmented frame?
    if (isFragmented(data, len)) {
        bool complete = handleFragment(data, len);
        if (!complete) {
            Serial.println("Frame fragment handled, waiting for more...");
            return false;
        }
        
        // frame is complete, use assembled data
        data = frameBuffer;
        len = currentSize;
        currentSize = 0;
        receivedFragments = 0;
    }

    // check minimum length
    Serial.printf("Decoding full frame data: %d bytes\n", len);
    if (len < FRAMEDEC_HEADER_SIZE) {
        Serial.printf("Frame too small: %d bytes\n", len);
        return false;
    }
    
    uint8_t deviceMode = data[0];
    uint8_t brightness = data[1];
    uint8_t paletteSize = min(data[2], (uint8_t)FRAMEDEC_MAX_PALETTE);
    uint8_t frameIndex = data[3];
    uint8_t frameTotal = data[4];
    uint16_t duration = data[6] | (data[7] << 8);
    if (duration < 1) duration = 1;

    // read palette
    Serial.printf("Palette size: %d colors\n", paletteSize);
    size_t paletteBytes = paletteSize * 3;
    if (len < FRAMEDEC_HEADER_SIZE + paletteBytes){       
        Serial.println("Incomplete palette data");
        return false;
    }

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
    size_t expectedSize = pixelStart + (MATRIX_HEIGHT * FRAMEDEC_BYTES_PER_ROW);
    if (len < expectedSize) {
        Serial.printf("Incomplete pixel data: %d < %d bytes\n", len, expectedSize);
        return false;
    }

    // loop on rows to decode pixels
    Serial.println("Decoding pixel data...");  
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