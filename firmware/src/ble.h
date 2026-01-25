#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include "config.h"
#include <Arduino.h>
#include <NimBLEDevice.h>

typedef void (*DataCallback)(uint8_t *data, size_t length);
typedef void (*DisconnectCallback)();

extern NimBLEServer *pServer;
extern NimBLECharacteristic *pCharData;
extern NimBLECharacteristic *pCharInfo;
extern bool deviceConnected;
extern DataCallback dataCallback;
extern DisconnectCallback disconnectCallback;
extern uint8_t currentBLEBrightness;
extern uint8_t currentClockColorIndex;
extern uint8_t currentClockGradientIndex;

class ServerCallbacks : public NimBLEServerCallbacks {
public:
  void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override;
  void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo,
                    int reason) override;
  void onMTUChange(uint16_t mtu, NimBLEConnInfo &connInfo) override;
};

class DataCallbacks : public NimBLECharacteristicCallbacks {
public:
  void onWrite(NimBLECharacteristic *pChar, NimBLEConnInfo &connInfo) override;
};

void initBLE(DataCallback callback, DisconnectCallback onDisconnect,
             uint8_t initialBrightness, uint8_t initialClockColor,
             uint8_t initialClockGradient);

#endif