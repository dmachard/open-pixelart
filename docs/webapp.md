# Web Control App

The Webapp uses the Web Bluetooth API to send data directly to your ESP32-C3.
Available at https://dmachard.github.io/open-pixelart/

⚠️ Note: The Web Control App works on **Android (Chrome)** and **desktop platforms (Windows, Linux) using Chrome**. 
Other browsers may not be supported.

<img src="../imgs/webapp_connect.png" width="500">
<img src="../imgs/webapp_mode.png" width="500">
<img src="../imgs/webapp_draw.png" width="500">

## Features

This web interface lets you:
- **Hardware Persistence**: All settings (brightness, default mode at startup, clock color) and the last displayed image are saved in the ESP32's NVS memory and restored automatically at boot.
- **Clock Mode**: Display current time with an animated edge-border seconds indicator and customizable digit colors (Lime, White, Cyan, Orange).
- Save/load drawings as `.json` files  
- Play animated slideshows

## Custom Drawings

Each drawing must be stored as a `.json` file in the `/drawings/` directory.  
Corresponding thumbnail images should be placed in the `/images/` directory.

You can generate a PNG thumbnail from any exported JSON drawing with the provided script:

```bash
cd web/
python3 -m venv venv
source venv/bin/activate
python -m pip install -r requirements.txt
python scripts/json_to_png.py drawings/led-matrix.json
```
