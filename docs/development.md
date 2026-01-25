# Development Setup

To build and flash the firmware locally, you need [PlatformIO Core](https://docs.platformio.org/en/latest/core/index.html).

## 1. Install PlatformIO
```bash
pip install -U platformio
```

## 2. Available Commands
A `Makefile` is provided for convenience:

*   **Build the firmware**:
    ```bash
    make build
    ```
*   **Flash the device**:
    (Ensure your ESP32 is connected via USB)
    ```bash
    make flash
    ```
*   **Clean build artifacts**:
    ```bash
    make clean
    ```
*   **Monitor serial output**:
    ```bash
    make monitor
    ```
