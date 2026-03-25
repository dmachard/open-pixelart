#include "ble.h"
#include "nightlight.h"

NimBLEServer *pServer = nullptr;
NimBLECharacteristic *pCharData = nullptr;
NimBLECharacteristic *pCharInfo = nullptr;
bool deviceConnected = false;
DataCallback dataCallback = nullptr;
DisconnectCallback disconnectCallback = nullptr;
uint8_t currentBLEBrightness = 25;
uint8_t currentClockColorIndex = 0;
uint8_t currentClockGradientIndex = 0;

void ServerCallbacks::onConnect(NimBLEServer *pServer,
                                NimBLEConnInfo &connInfo) {
  deviceConnected = true;
  Serial.printf("✓ Client connected (initial MTU: %d bytes)\n",
                connInfo.getMTU());

  const NightLightConfig &nl = getNightLightConfig();

  // Send device info upon connection
  String deviceInfo =
      String("{\"model\":\"") + DEVICE_MODEL + "\",\"width\":" + MATRIX_WIDTH +
      ",\"height\":" + MATRIX_HEIGHT +
      ",\"brightness\":" + currentBLEBrightness +
      ",\"clockColorIndex\":" + currentClockColorIndex +
      ",\"clockGradientIndex\":" + currentClockGradientIndex +
      ",\"nlEnabled\":" + (nl.enabled ? 1 : 0) +
      ",\"nlStartH\":" + nl.startHour + ",\"nlStartM\":" + nl.startMinute +
      ",\"nlEndH\":" + nl.endHour + ",\"nlEndM\":" + nl.endMinute +
      ",\"nlColor\":" + nl.colorIndex + ",\"nlBrightness\":" + nl.brightness +
      ",\"mtu\":" + connInfo.getMTU() + "}";
  pCharInfo->setValue(deviceInfo.c_str());
  pCharInfo->notify();
  Serial.println("→ Device info sent: " + deviceInfo);
}

void ServerCallbacks::onDisconnect(NimBLEServer *pServer,
                                   NimBLEConnInfo &connInfo, int reason) {
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

void ServerCallbacks::onMTUChange(uint16_t mtu, NimBLEConnInfo &connInfo) {
  Serial.printf("MTU changed to: %d bytes\n", mtu);

  // Resend device info with updated MTU
  if (pCharInfo && deviceConnected) {
    String deviceInfo = String("{\"model\":\"") + DEVICE_MODEL +
                        "\",\"width\":" + MATRIX_WIDTH +
                        ",\"height\":" + MATRIX_HEIGHT +
                        ",\"brightness\":" + currentBLEBrightness +
                        ",\"clockColorIndex\":" + currentClockColorIndex +
                        ",\"clockGradientIndex\":" + currentClockGradientIndex +
                        ",\"mtu\":" + mtu + "}";
    pCharInfo->setValue(deviceInfo.c_str());
    pCharInfo->notify();
    Serial.println("Updated device info sent with new MTU");
  }
}

void DataCallbacks::onWrite(NimBLECharacteristic *pChar,
                            NimBLEConnInfo &connInfo) {
  std::string value = pChar->getValue();
  Serial.printf("Data received: %d bytes\n", value.length());

  if (dataCallback) {
    dataCallback((uint8_t *)value.data(), value.length());
  }
}

void initBLE(DataCallback callback, DisconnectCallback onDisconnect,
             uint8_t initialBrightness, uint8_t initialClockColor,
             uint8_t initialClockGradient) {
  dataCallback = callback;
  disconnectCallback = onDisconnect;
  currentBLEBrightness = initialBrightness;
  currentClockColorIndex = initialClockColor;
  currentClockGradientIndex = initialClockGradient;

  Serial.println("Initializing NimBLE...");

  NimBLEDevice::init(BLE_DEVICE_NAME);

  // Configuration NimBLE pour optimiser la mémoire
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);            // Max power
  NimBLEDevice::setSecurityAuth(false, false, true); // No bonding
  NimBLEDevice::setMTU(517);                         // Ask for max MTU size

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  NimBLEService *pService = pServer->createService(SERVICE_UUID);

  // Characteristic for frame data (WRITE)
  pCharData =
      pService->createCharacteristic(CHAR_DATA_UUID, NIMBLE_PROPERTY::WRITE);
  pCharData->setCallbacks(new DataCallbacks());
  pCharData->createDescriptor("2901", NIMBLE_PROPERTY::READ)->setValue("Frame Data");

  // Characteristic for device info (READ + NOTIFY)
  pCharInfo = pService->createCharacteristic(
      CHAR_INFO_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  pCharInfo->createDescriptor("2901", NIMBLE_PROPERTY::READ)->setValue("Device Info");

  // Define device info
  String deviceInfo =
      String("{\"model\":\"") + DEVICE_MODEL + "\",\"width\":" + MATRIX_WIDTH +
      ",\"height\":" + MATRIX_HEIGHT +
      ",\"brightness\":" + currentBLEBrightness +
      ",\"clockColorIndex\":" + currentClockColorIndex +
      ",\"clockGradientIndex\":" + currentClockGradientIndex + "}";
  pCharInfo->setValue(deviceInfo.c_str());

  pService->start();

  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  
  // Primary advertising data (max 31 bytes)
  NimBLEAdvertisementData advData;
  advData.setName(BLE_DEVICE_NAME); // Adds Complete Local Name (0x09)
  advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP); // General Discoverable, BR/EDR Not Supported

  // Scan response data (additional 31 bytes)
  NimBLEAdvertisementData scanResponseData;
  scanResponseData.setCompleteServices(NimBLEUUID(SERVICE_UUID));
  
  // Set both datasets
  pAdvertising->setAdvertisementData(advData);
  pAdvertising->setScanResponseData(scanResponseData);
  pAdvertising->start();

  Serial.printf("✓ NimBLE started - Model: %s (%dx%d)\n", DEVICE_MODEL,
                MATRIX_WIDTH, MATRIX_HEIGHT);
}