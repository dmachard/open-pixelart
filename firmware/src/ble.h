#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include "config.h"

typedef void (*DataCallback)(uint8_t* data, size_t length);
typedef void (*DisconnectCallback)();

extern BLEServer* pServer;
extern BLECharacteristic* pCharData;
extern BLECharacteristic* pCharInfo;
extern bool deviceConnected;

extern DataCallback dataCallback;
extern DisconnectCallback disconnectCallback;

class ServerCallbacks: public BLEServerCallbacks {
public:
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;
};

class DataCallbacks: public BLECharacteristicCallbacks {
public:
    void onWrite(BLECharacteristic *pChar) override;
};

void initBLE(DataCallback callback, DisconnectCallback onDisconnect = nullptr);

#endif