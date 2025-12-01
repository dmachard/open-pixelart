#include <Arduino.h>
#include <FastLED.h>

#include "config.h"
#include "display.h"
#include "ble.h"
#include "decoder.h"

#define MAX_FRAMES 15
#define MODE_DRAW 0
#define MODE_GALLERY 1

Frame frames[MAX_FRAMES];
uint8_t frameCount = 0;
uint8_t currentFrame = 0;
unsigned long lastFrameChange = 0;

void updateDisplay() {
    if (frameCount == 0) return;

    unsigned long now = millis();
    Frame& f = frames[currentFrame];

    if (now - lastFrameChange >= f.duration_ms) {
        currentFrame = (currentFrame + 1) % frameCount;
        lastFrameChange = now;
        displayFrame(frames[currentFrame]);
    }
}

void onIncomingData(uint8_t* data, size_t len) {
    Frame f;
    if (!FrameDecoder::decode(f, data, len)) {
        Serial.println("Failed to decode frame");
        return;
    }

    switch (f.deviceMode) {
        case MODE_DRAW:
            frameCount = 1;
            currentFrame = 0;
            frames[0] = f;
            
            lastFrameChange = millis();
            displayFrame(frames[0]);
            Serial.println("Switched to DRAW Mode.");
            break;
        case MODE_GALLERY:
            // On first frame, set total frame count
            if (f.frameIndex == 0) {
                frameCount = min((uint8_t)f.frameTotal, (uint8_t)MAX_FRAMES);
                currentFrame = 0;
            }
            
            // Store the received frame at its index
            if (f.frameIndex < frameCount) { 
                frames[f.frameIndex] = f;
            }

            // Start slideshow if last frame received
            if (f.frameIndex + 1 == f.frameTotal && frameCount > 0) {
                lastFrameChange = millis();
                displayFrame(frames[0]); 
                Serial.printf("Switched to GALLERY Mode: %d frames ready.\n", frameCount);
            }
            break;
        default:
            Serial.printf("Unknown Mode received: %d\n", f.deviceMode);
            frameCount = 0;
            break;
    }
}

void onClientDisconnect() {
    frameCount = 0;
    currentFrame = 0;
    clearDisplay();
}

void setup() {
    // Start Serial for debugging
    Serial.begin(115200);

    // Initialize display
    initDisplay();

    // Initialize BLE with callbacks
    initBLE(onIncomingData, onClientDisconnect);
}

void loop() {
    // Continuously update display
    updateDisplay();

    // Allow background tasks to run
    yield(); 
}
