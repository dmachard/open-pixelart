#include "ble.h"

BLEServer* pServer = nullptr;
BLECharacteristic* pCharData = nullptr;
BLECharacteristic* pCharInfo = nullptr;
bool deviceConnected = false;

DataCallback dataCallback = nullptr;
DisconnectCallback disconnectCallback = nullptr;

void ServerCallbacks::onConnect(BLEServer* pServer) {
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

void ServerCallbacks::onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("✗ Client disconnected");

    // callbacks for disconnect
    if (disconnectCallback) {
        disconnectCallback();
    }

    // pServer->startAdvertising() doit être appelé sur le serveur, pas sur pServer->getAdvertising()
    BLEDevice::startAdvertising(); // Utilisation de la fonction globale ou la bonne méthode sur le serveur
    // Note : pServer->startAdvertising(); est la bonne méthode pour la lib ESP32 BLE
    pServer->startAdvertising(); 
    Serial.println("✓ BLE restarted");
}

void DataCallbacks::onWrite(BLECharacteristic *pChar) {
    std::string value = pChar->getValue();
    Serial.printf("Data received: %d bytes\n", value.length());
    
    if (dataCallback) {
        dataCallback((uint8_t*)value.data(), value.length());
    }
}

void initBLE(DataCallback callback, DisconnectCallback onDisconnect) {
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
                      "\",\"height\":" + MATRIX_HEIGHT + "}";
    pCharInfo->setValue(deviceInfo.c_str());
    
    pService->start();
    
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();
    
    Serial.printf("✓ BLE started - Model: %s (%dx%d)\n", 
                  DEVICE_MODEL, MATRIX_WIDTH, MATRIX_HEIGHT);
}