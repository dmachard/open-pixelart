#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include "config.h"

typedef void (*DataCallback)(uint8_t* data, size_t length);
typedef void (*DisconnectCallback)();

BLEServer* pServer = nullptr;
BLECharacteristic* pCharData = nullptr;
BLECharacteristic* pCharInfo = nullptr;
bool deviceConnected = false;

DataCallback dataCallback = nullptr;
DisconnectCallback disconnectCallback = nullptr;

class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("✓ Client connected");
        
        // send device info upon connection
        String deviceInfo = String("{\"model\":\"") + DEVICE_MODEL + 
                          "\",\"width\":" + MATRIX_WIDTH + 
                          ",\"height\":" + MATRIX_HEIGHT + "}";
        
        pCharInfo->setValue(deviceInfo.c_str());
        pCharInfo->notify();
        
        Serial.println("→ Device info sent: " + deviceInfo);
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("✗ Client disconnected");

        // callbacks for disconnect
        if (disconnectCallback) {
            disconnectCallback();
        }

        pServer->startAdvertising();
        Serial.println("✓ BLE restarted");
    }
};

class DataCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pChar) {
        std::string value = pChar->getValue();
        Serial.printf("Data received: %d bytes\n", value.length());
        
        if (dataCallback) {
            dataCallback((uint8_t*)value.data(), value.length());
        }
    }
};

void initBLE(DataCallback callback, DisconnectCallback onDisconnect = nullptr) {
    dataCallback = callback;
    disconnectCallback = onDisconnect;
    Serial.println("Initializing BLE...");
    
    BLEDevice::init(BLE_DEVICE_NAME);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    
    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    // Characteristic for frame data (WRITE)
    pCharData = pService->createCharacteristic(
        CHAR_DATA_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pCharData->setCallbacks(new DataCallbacks());
    
    // Characteristic for device info (READ + NOTIFY)
    pCharInfo = pService->createCharacteristic(
        CHAR_INFO_UUID,
        BLECharacteristic::PROPERTY_READ | 
        BLECharacteristic::PROPERTY_NOTIFY
    );
    
    // Define device info
    String deviceInfo = String("{\"model\":\"") + DEVICE_MODEL + 
                      "\",\"width\":" + MATRIX_WIDTH + 
                      ",\"height\":" + MATRIX_HEIGHT + "}";
    pCharInfo->setValue(deviceInfo.c_str());
    
    pService->start();
    
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();
    
    Serial.printf("✓ BLE started - Model: %s (%dx%d)\n", 
                  DEVICE_MODEL, MATRIX_WIDTH, MATRIX_HEIGHT);
}

#endif
