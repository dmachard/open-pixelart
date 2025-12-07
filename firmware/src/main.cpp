#include <Arduino.h>

#include "config.h"
#include "display.h"
#include "ble.h"
#include "decoder.h"

#define MAX_FRAMES 15
#define MODE_DRAW 0
#define MODE_GALLERY 1

FrameDecoder decoder;
Frame decodedFrame;
Frame frames[MAX_FRAMES];

uint8_t frameCount = 0;
uint8_t currentFrameIndex = 0;
unsigned long lastFrameChange = 0;


void updateDisplay() {
    if (frameCount == 0) return;

    unsigned long now = millis();
    Frame& f = frames[currentFrameIndex];

    if (now - lastFrameChange >= f.duration_ms) {
        currentFrameIndex = (currentFrameIndex + 1) % frameCount;
        lastFrameChange = now;
        displayFrame(frames[currentFrameIndex]);
    }
}

void onIncomingData(uint8_t* data, size_t len) {

    if (!decoder.decode(decodedFrame, data, len)) {
        Serial.println("Failed to decode frame");
        return;
    }

    switch (decodedFrame.deviceMode) {
        case MODE_DRAW:
            frameCount = 1;
            currentFrameIndex = 0;
            frames[0] = decodedFrame;
            
            lastFrameChange = millis();
            displayFrame(frames[0]);
            Serial.println("Switched to DRAW Mode.");
            break;
        case MODE_GALLERY:
            // On first frame, set total frame count
            if (decodedFrame.frameIndex == 0) {
                frameCount = min((uint8_t)decodedFrame.frameTotal, (uint8_t)MAX_FRAMES);
                currentFrameIndex = 0;
            }
            
            // Store the received frame at its index
            if (decodedFrame.frameIndex < frameCount) { 
                frames[decodedFrame.frameIndex] = decodedFrame;
            }

            // Start slideshow if last frame received
            if (decodedFrame.frameIndex + 1 == decodedFrame.frameTotal && frameCount > 0) {
                lastFrameChange = millis();
                displayFrame(frames[0]); 
                Serial.printf("Switched to GALLERY Mode: %d frames ready.\n", frameCount);
            }
            break;
        default:
            Serial.printf("Unknown Mode received: %d\n", decodedFrame.deviceMode);
            frameCount = 0;
            break;
    }
}

void onClientDisconnect() {
    frameCount = 0;
    currentFrameIndex = 0;
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
