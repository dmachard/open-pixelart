#ifndef CONFIG_H
#define CONFIG_H

// Select device type according to your hardware setup
#define USE_WS2812
// #define USE_HUB75

// Config BLE
#define BLE_DEVICE_NAME "OpenPixelArt"
#define SERVICE_UUID "12345678-1234-1234-1234-123456789012"
#define CHAR_DATA_UUID "87654321-4321-4321-4321-210987654321"
#define CHAR_INFO_UUID "12345678-4321-1234-4321-123456789012"

// Device-specific configurations
#ifdef USE_WS2812
    #define DEVICE_MODEL "WS2812"
    #define LED_DATA_PIN 8
    #define MATRIX_WIDTH 16
    #define MATRIX_HEIGHT 16
    #define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT)
#endif

// RTC (DS3231) Configuration
#define RTC_SDA_PIN 6
#define RTC_SCL_PIN 7

#endif