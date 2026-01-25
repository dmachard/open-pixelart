# Hardware Setup

- **ESP32-C3 Super mini**:  Microcontroller
- **WS2812B LED Matrix 16×16**: 256 addressable RGB LEDs
- **DS3231 RTC Module**: High-precision real-time clock sensor
- **Power Supply**:  5 V DC, ≥ 1 A recommended
- **3D Printed Enclosure**: STL and FreeCAD files included in `/enclosure/`

## Wiring

| Signal | ESP32-C3 Pin | Notes |
|--------|--------------|-------|
| LED Data | GPIO 8 | Configurable via `LED_DATA_PIN` |
| RTC SDA | GPIO 6 | I2C Data line for DS3231 |
| RTC SCL | GPIO 7 | I2C Clock line for DS3231 |
| 5V | VIN | Power input |
| GND | GND | Common ground |

<img src="../imgs/circuit.png" title="Wiring Diagram">
