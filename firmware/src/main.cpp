#include <Arduino.h>
#include <FastLED.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID "12345678-1234-1234-1234-123456789012"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-210987654321"
#define DATA_PIN 8
#define WIDTH 16
#define HEIGHT 16
#define NUM_LEDS (WIDTH * HEIGHT)
#define MAX_BRIGHTNESS 255
#define COLOR_ORDER GRB
#define LED_TYPE WS2812B
#define FRAME_DELAY 15  // 15 seconds per frame
#define RANDOM_ANIMATION_DELAY 30  // 30 ms per pixel reveal
#define BYTES_PER_ROW (WIDTH / 2)
#define MAX_PALETTE_SIZE 16
#define MAX_FRAMES 10
#define HEADER_SIZE 8

// LED arrays
CRGB leds[NUM_LEDS];
CRGB palette[MAX_PALETTE_SIZE];
uint8_t brightness = 25;

// BLE
BLECharacteristic *pCharacteristic;
BLEServer *pServer;
bool deviceConnected = false;

// Frame storage for slideshow
CRGB** frames = nullptr;
int totalFrames = 0;  // Total frames expected for current slideshow
int currentFrame = 0;
unsigned long lastFrameChange = 0;
int receivedFrames = 0;  // Number of frames actually received
uint16_t frameDuration = 15;  // Frame duration in seconds

// Transition system
enum TransitionMode { TR_PROGRESSIVE = 0, TR_INSTANT = 1 };
enum DisplayMode { MODE_DRAW = 0, MODE_GALLERY = 1 };

struct TransitionState {
    bool active;
    TransitionMode mode;
    CRGB* targetFrame;  // Will be dynamically allocated
    uint16_t* remainingPixels;  // Changed to uint16_t and dynamic
    int remainingCount;
    unsigned long lastUpdate;
};

TransitionState transition;
TransitionMode transMode = TR_PROGRESSIVE;

// Convert logical (x,y) coordinates to physical LED strip index
inline int getPhysicalLedIndex(int x, int y) {
    int physicalX = (WIDTH - 1) - x;
    if (y % 2 == 0) {
        return y * WIDTH + physicalX;
    } else {
        return y * WIDTH + ((WIDTH - 1) - physicalX);
    }
}

// Forward declarations
void freeFrames();
void freeTransition();

// Initialize transition buffers
void initTransition() {
    if (transition.targetFrame == nullptr) {
        transition.targetFrame = (CRGB*)malloc(NUM_LEDS * sizeof(CRGB));
    }
    if (transition.remainingPixels == nullptr) {
        transition.remainingPixels = (uint16_t*)malloc(NUM_LEDS * sizeof(uint16_t));
    }
}

// Free transition buffers
void freeTransition() {
    if (transition.targetFrame) {
        free(transition.targetFrame);
        transition.targetFrame = nullptr;
    }
    if (transition.remainingPixels) {
        free(transition.remainingPixels);
        transition.remainingPixels = nullptr;
    }
}

// Allocate frames for slideshow
bool allocateFrames(int count) {
    freeFrames();
    
    frames = (CRGB**)malloc(count * sizeof(CRGB*));
    if (frames == nullptr) {
        Serial.println("ERROR: Cannot allocate frame pointers");
        return false;
    }
    
    for (int i = 0; i < count; i++) {
        frames[i] = (CRGB*)malloc(NUM_LEDS * sizeof(CRGB));
        if (frames[i] == nullptr) {
            Serial.printf("ERROR: Cannot allocate frame %d\n", i);
            // Free what we allocated so far
            for (int j = 0; j < i; j++) {
                free(frames[j]);
            }
            free(frames);
            frames = nullptr;
            return false;
        }
    }
    
    return true;
}

// Free all frames
void freeFrames() {
    if (frames) {
        for (int i = 0; i < totalFrames; i++) {
            if (frames[i]) {
                free(frames[i]);
            }
        }
        free(frames);
        frames = nullptr;
    }
    totalFrames = 0;
    receivedFrames = 0;
}

void displayFrame(CRGB* frame, TransitionMode mode) {
    if (!transition.targetFrame) {
        initTransition();
    }
    
    // copy target frame
    memcpy(transition.targetFrame, frame, NUM_LEDS * sizeof(CRGB));

    transition.active = true;
    transition.mode = mode;
    transition.lastUpdate = millis();
    
    if (mode == TR_PROGRESSIVE) {
        transition.remainingCount = NUM_LEDS;
        for (int i = 0; i < NUM_LEDS; i++) {
            transition.remainingPixels[i] = i;
        }
    } else if (mode == TR_INSTANT) {
        memcpy(leds, transition.targetFrame, NUM_LEDS * sizeof(CRGB));
        FastLED.show();
        transition.active = false;
    }
}

void updateDisplay() {
    unsigned long now = millis();

    if (transition.active && transition.mode == TR_PROGRESSIVE) {
        if (now - transition.lastUpdate >= RANDOM_ANIMATION_DELAY) {
            transition.lastUpdate = now;
            
            if (transition.remainingCount > 0) {
                int r = random(transition.remainingCount);
                int idx = transition.remainingPixels[r];
                leds[idx] = transition.targetFrame[idx];
                FastLED.show();
                
                transition.remainingPixels[r] = transition.remainingPixels[transition.remainingCount - 1];
                transition.remainingCount--;
            } else {
                transition.active = false;
            }
        }
        return;
    }
    
    // Update slideshow (only when no transition active)
    if (totalFrames > 0 && frames && now - lastFrameChange >= (frameDuration * 1000UL)) {
        lastFrameChange = now;
        currentFrame = (currentFrame + 1) % totalFrames;
  
        // Use the global transition mode
        displayFrame(frames[currentFrame], transMode);
    }
}

void processFrameData(uint8_t* data, size_t len) {
    if (len < HEADER_SIZE) {
        Serial.println("Data too short");
        return;
    }
    
    // Parse header
    uint8_t mode = data[0];
    brightness = data[1];
    FastLED.setBrightness(brightness);
    uint8_t paletteSize = min(data[2], (uint8_t)MAX_PALETTE_SIZE);
    uint8_t frameIndex = data[3];
    uint8_t frameCount = data[4];
    transMode = (TransitionMode)data[5];  // Update global transMode
    frameDuration = data[6] | (data[7] << 8);  // Read uint16_t (little-endian)
    
    // Clamp frame duration to reasonable values
    if (frameDuration < 1) frameDuration = 1;
    if (frameDuration > 300) frameDuration = FRAME_DELAY;
    
    // Parse palette
    size_t paletteBytes = paletteSize * 3;
    if (len < HEADER_SIZE + paletteBytes) return;
    
    for (int i = 0; i < paletteSize; i++) {
        int offset = HEADER_SIZE + i * 3;
        palette[i] = CRGB(data[offset], data[offset+1], data[offset+2]);
    }
    
    // Parse pixels into temporary buffer
    CRGB* tempFrame = (CRGB*)malloc(NUM_LEDS * sizeof(CRGB));
    if (!tempFrame) {
        Serial.println("ERROR: Cannot allocate temp frame");
        return;
    }
    
    size_t pixelDataStart = HEADER_SIZE + paletteBytes;
    
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x += 2) {
            int byteIndex = pixelDataStart + (y * BYTES_PER_ROW + x / 2);
            if (byteIndex >= len) {
                free(tempFrame);
                return;
            }
            
            uint8_t pixelByte = data[byteIndex];
            uint8_t idx1 = (pixelByte >> 4) & 0x0F;
            uint8_t idx2 = pixelByte & 0x0F;
            
            tempFrame[getPhysicalLedIndex(x, y)] = (idx1 < paletteSize) ? palette[idx1] : CRGB::Black;
            tempFrame[getPhysicalLedIndex(x + 1, y)] = (idx2 < paletteSize) ? palette[idx2] : CRGB::Black;
        }
    }
    
    Serial.printf("Frame %d/%d received (mode: %s, duration: %ds)\n", 
                  frameIndex + 1, frameCount, 
                  transMode == TR_INSTANT ? "INSTANT" : "PROGRESSIVE",
                  frameDuration);
    
    // Handle mode
    if (mode == MODE_DRAW) {
        freeFrames();  // Stop slideshow and free memory
        displayFrame(tempFrame, transMode);
    } else if (mode == MODE_GALLERY) {
        if (frameIndex >= MAX_FRAMES) {
            Serial.println("Too many frames!");
            free(tempFrame);
            return;
        }
        
        // If this is a new slideshow (frameIndex 0 or different frameCount), reset everything
        if (frameIndex == 0 || frameCount != totalFrames || !frames) {
            Serial.println("New slideshow detected, resetting...");
            freeFrames();
            
            if (!allocateFrames(frameCount)) {
                Serial.println("ERROR: Cannot allocate frames");
                free(tempFrame);
                return;
            }
            totalFrames = frameCount;
            receivedFrames = 0;
        }
        
        // Verify frames array exists
        if (!frames || frameIndex >= totalFrames || !frames[frameIndex]) {
            Serial.printf("ERROR: Frame storage not ready for frame %d\n", frameIndex);
            free(tempFrame);
            return;
        }
        
        // Copy temp frame to storage
        memcpy(frames[frameIndex], tempFrame, NUM_LEDS * sizeof(CRGB));
        receivedFrames++;
        
        // Start slideshow ONLY when all frames received
        if (receivedFrames == totalFrames) {
            currentFrame = 0;
            lastFrameChange = millis();
            
            // Now it's safe to display frames[0]
            if (frames && frames[0]) {
                displayFrame(frames[0], transMode);
                Serial.printf("Slideshow started: %d frames (mode: %s)\n", 
                             totalFrames,
                             transMode == TR_INSTANT ? "INSTANT" : "PROGRESSIVE");
            }
        } else {
            Serial.printf("Progress: %d/%d frames received\n", receivedFrames, totalFrames);
        }
    }
    
    free(tempFrame);
}

class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("✓ Client connected");
    }
    
    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("✗ Client disconnected");
        
        // Free memory on disconnect
        freeFrames();
        freeTransition();
        
        delay(500);
        pServer->startAdvertising();
        Serial.println("✓ BLE restarted");
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        Serial.printf("Data received: %d bytes\n", value.length());
        processFrameData((uint8_t*)value.data(), value.length());
    }
};

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("Starting ESP32-C3...");
    
    FastLED.addLeds<WS2812B, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(brightness);
    FastLED.clear();
    FastLED.show();
    Serial.println("FastLED Matrix Ready");
    
    // Initialize transition buffers
    initTransition();
    
    Serial.println("Initing BLE...");
    BLEDevice::init("Matrix16x16");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pCharacteristic->setCallbacks(new MyCallbacks());
    
    pService->start();
    
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    pAdvertising->start();
    
    Serial.println("✓ BLE started");
    Serial.println("✓ Name: Matrix16x16");
    Serial.println("✓ Waiting for connection...");
    
    transition.active = false;
}

void loop() {
    updateDisplay();
}