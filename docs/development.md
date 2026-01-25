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

## 3. Web App Development

The web application is located in the `web/` directory.

### Structure
- `web/pages/`: Contains HTML components for each screen.
- `web/index_template.html`: Main template.
- `web/index.html`: **Generated file** (do not edit manually).

### Commands

*   **Build the web app**:
    Reassembles `index.html` from components.
    ```bash
    make build-web
    ```

*   **Run locally (Docker)**:
    Starts a local Nginx server at [http://localhost:8080](http://localhost:8080).
    ```bash
    make run-web
    ```
