#include "ble.h"

NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pCharData = nullptr;
NimBLECharacteristic* pCharInfo = nullptr;
bool deviceConnected = false;
DataCallback dataCallback = nullptr;
DisconnectCallback disconnectCallback = nullptr;

void ServerCallbacks::onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
    deviceConnected = true;
    Serial.println("✓ Client connected");
    
    // Send device info upon connection
    String deviceInfo = String("{\"model\":\"") + DEVICE_MODEL + 
                       "\",\"width\":" + MATRIX_WIDTH + 
                       ",\"height\":" + MATRIX_HEIGHT + "}";
    pCharInfo->setValue(deviceInfo.c_str());
    pCharInfo->notify();
    Serial.println("→ Device info sent: " + deviceInfo);
}

void ServerCallbacks::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
    deviceConnected = false;
    Serial.printf("✗ Client disconnected (reason: %d)\n", reason);
    
    // Callback for disconnect
    if (disconnectCallback) {
        disconnectCallback();
    }
    
    // Restart advertising
    NimBLEDevice::startAdvertising();
    Serial.println("✓ BLE restarted");
}

void DataCallbacks::onWrite(NimBLECharacteristic *pChar, NimBLEConnInfo& connInfo) {
    std::string value = pChar->getValue();
    Serial.printf("Data received: %d bytes\n", value.length());
    
    if (dataCallback) {
        dataCallback((uint8_t*)value.data(), value.length());
    }
}

void initBLE(DataCallback callback, DisconnectCallback onDisconnect) {
    dataCallback = callback;
    disconnectCallback = onDisconnect;
    
    Serial.println("Initializing NimBLE...");
    
    NimBLEDevice::init(BLE_DEVICE_NAME);
    
    // Configuration NimBLE pour optimiser la mémoire
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // Max power
    NimBLEDevice::setSecurityAuth(false, false, true); // No bonding
    
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    
    NimBLEService *pService = pServer->createService(SERVICE_UUID);
    
    // Characteristic for frame data (WRITE)
    pCharData = pService->createCharacteristic(
        CHAR_DATA_UUID,
        NIMBLE_PROPERTY::WRITE
    );
    pCharData->setCallbacks(new DataCallbacks());
    
    // Characteristic for device info (READ + NOTIFY)
    pCharInfo = pService->createCharacteristic(
        CHAR_INFO_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    
    // Define device info
    String deviceInfo = String("{\"model\":\"") + DEVICE_MODEL + 
                       "\",\"width\":" + MATRIX_WIDTH + 
                       ",\"height\":" + MATRIX_HEIGHT + "}";
    pCharInfo->setValue(deviceInfo.c_str());
    
    pService->start();
    
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->start();
    
    Serial.printf("✓ NimBLE started - Model: %s (%dx%d)\n", 
                  DEVICE_MODEL, MATRIX_WIDTH, MATRIX_HEIGHT);
}