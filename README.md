# PixelBox

A Bluetooth-controlled PixelBox project powered by ESP32.
- Draw pixel art instantly (16x16)
- Play animations
- Display a clock
- Tetris
- Snake
- Flappy Bird
- Arkanoid

<img src="imgs/clock.png" title="demo" width="400">

## Demo

▶️ [See video for clock mode](https://github.com/user-attachments/assets/048136df-9e7c-436d-b4ce-061c26e4e01e)


## Hardware setup

- **ESP32-C3 Super mini**:  Microcontroller
- **WS2812B LED Matrix 16×16**: 256 addressable RGB LEDs
- **DS3231 RTC Module**: High-precision real-time clock sensor
- **Power Supply**:  5 V DC, ≥ 1 A recommended
- **3D Printed Enclosure**: STL and FreeCAD files included in `/enclosure/`

| Signal | ESP32-C3 Pin | Notes |
|--------|--------------|-------|
| LED Data | GPIO 8 | Configurable via `LED_DATA_PIN` |
| RTC SDA | GPIO 6 | I2C Data line for DS3231 |
| RTC SCL | GPIO 7 | I2C Clock line for DS3231 |
| 5V | VIN | Power input |
| GND | GND | Common ground |

<img src="imgs/circuit.png" title="demo">

## Firmware (ESP32-C3 + WS2812B)

Firmware for controlling a 16×16 WS2812B LED matrix using an ESP32-C3 via **Bluetooth Low Energy (BLE)**.  
Designed to work with the **Web Control App**, which lets you draw and upload pixel art or animations wirelessly.

**Advertised Bluetooth device name:** `OpenPixelArt`

## Web Control App

The Webapp uses the Web Bluetooth API to send data directly to your ESP32-C3.
Available at https://dmachard.github.io/open-pixelart/

⚠️ Note: The Web Control App works on **Android (Chrome)** and **desktop platforms (Windows, Linux) using Chrome**. 
Other browsers may not be supported.

<img src="imgs/webapp_connect.png" width="500">
<img src="imgs/webapp_mode.png" width="500">
<img src="imgs/webapp_draw.png" width="500">

This web interface lets you:
- **Hardware Persistence**: All settings (brightness, default mode at startup, clock color) and the last displayed image are saved in the ESP32's NVS memory and restored automatically at boot.
- **Clock Mode**: Display current time with an animated edge-border seconds indicator and customizable digit colors (Lime, White, Cyan, Orange).
- Save/load drawings as `.json` files  
- Play animated slideshows

Each drawing must be stored as a `.json` file in the `/drawings/` directory.  
Corresponding thumbnail images should be placed in the `/images/` directory.

You can generate a PNG thumbnail from any exported JSON drawing with the provided script:

```
cd web/
python3 -m venv venv
source venv/bin/activate
python -m pip install -r requirements.txt
python scripts/json_to_png.py drawings/led-matrix.json
```

## Gallery

<table>

<tr>
<td><img src="web/images/alien.png" width="50" title="alien"></td>
<td><img src="web/images/bear.png" width="50" title="bear"></td>
<td><img src="web/images/bird.png" width="50" title="bird"></td>
<td><img src="web/images/bot.png" width="50" title="bot"></td>
<td><img src="web/images/buzz.png" width="50" title="buzz"></td>
<td><img src="web/images/cat1.png" width="50" title="cat"></td>
<td><img src="web/images/cat2.png" width="50" title="cat"></td>
<td><img src="web/images/cat3.png" width="50" title="cat"></td>
</tr>

<tr>
<td><img src="web/images/chicken.png" width="50" title="chicken"></td>
<td><img src="web/images/city.png" width="50" title="city"></td>
<td><img src="web/images/face1.png" width="50" title="face"></td>
<td><img src="web/images/face2.png" width="50" title="face"></td>
<td><img src="web/images/face3.png" width="50" title="face"></td>
<td><img src="web/images/fox1.png" width="50" title="fox"></td>
<td><img src="web/images/fox2.png" width="50" title="fox"></td>
<td><img src="web/images/bear2.png" width="50" title="bear"></td>
</tr>

<tr>
<td><img src="web/images/frog.png" width="50" title="frog"></td>
<td><img src="web/images/ghost1.png" width="50" title="ghost"></td>
<td><img src="web/images/ghost2.png" width="50" title="ghost"></td>
<td><img src="web/images/hand.png" width="50" title="hand"></td>
<td><img src="web/images/heart.png" width="50" title="heart"></td>
<td><img src="web/images/hellokitty.png" width="50" title="hellokitty"></td>
<td><img src="web/images/home.png" width="50" title="home"></td>
<td><img src="web/images/iloveyou.png" width="50" title="iloveyou"></td>
</tr>

<tr>
<td><img src="web/images/mario.png" width="50" title="mario"></td>
<td><img src="web/images/minion.png" width="50" title="minion"></td>
<td><img src="web/images/pacman.png" width="50" title="pacman"></td>
<td><img src="web/images/pinkflamingo.png" width="50" title="pinkflamingo"></td>
<td><img src="web/images/pockemon.png" width="50" title="pockemon"></td>
<td><img src="web/images/pumpkin.png" width="50" title="pumpkin"></td>
<td><img src="web/images/question.png" width="50" title="question"></td>
<td><img src="web/images/skull.png" width="50" title="skull"></td>
</tr>

<tr>
<td><img src="web/images/smiley1.png" width="50" title="smiley"></td>
<td><img src="web/images/smiley2.png" width="50" title="smiley"></td>
<td><img src="web/images/smiley3.png" width="50" title="smiley"></td>
<td><img src="web/images/smiley4.png" width="50" title="smiley"></td>
<td><img src="web/images/sonic.png" width="50" title="sonic"></td>
<td><img src="web/images/yoshi.png" width="50" title="yoshi"></td>
<td><img src="web/images/smiley5.png" width="50" title="smiley"></td>
<td><img src="web/images/candle.png" width="50" title="candle"></td>
</tr>

<tr>
<td><img src="web/images/cat4.png" width="50" title="cat"></td>
<td><img src="web/images/cloud.png" width="50" title="cloud"></td>
<td><img src="web/images/dog.png" width="50" title="dog"></td>
<td><img src="web/images/franchektein.png" width="50" title="franchektein"></td>
<td><img src="web/images/moon.png" width="50" title="moon"></td>
<td><img src="web/images/moon2.png" width="50" title="moon"></td>
<td><img src="web/images/star.png" width="50" title="star"></td>
<td><img src="web/images/whale.png" width="50" title="whale"></td>
</tr>

<tr>
<td><img src="web/images/bot2.png" width="50" title="bot"></td>
<td><img src="web/images/skull2.png" width="50" title="skull"></td>
<td><img src="web/images/counter.png" width="50" title="counter"></td>
<td><img src="web/images/ironman3.png" width="50" title="ironman"></td>
<td><img src="web/images/rocket.png" width="50" title="rocket"></td>
<td><img src="web/images/goomba.png" width="50" title="goomba"></td>
<td><img src="web/images/clock.png" width="50" title="clock"></td>
<td><img src="web/images/apple.png" width="50" title="apple"></td>
</tr>

<tr>
<td><img src="web/images/playstation.png" width="50" title="playstation"></td>
<td><img src="web/images/dog2.png" width="50" title="dog"></td>
<td><img src="web/images/pikachu.png" width="50" title="pikachu"></td>
<td><img src="web/images/stitch.png" width="50" title="stitch"></td>
<td><img src="web/images/ironman2.png" width="50" title="ironman"></td>
<td><img src="web/images/phone.png" width="50" title="phone"></td>
<td><img src="web/images/ghostbuster.png" width="50" title="ghost buster"></td>
<td><img src="web/images/unicorn.png" width="50" title="unicorn"></td>
</tr>

<tr>
<td><img src="web/images/christmastree.png" width="50" title="christmas tree"></td>
<td><img src="web/images/note.png" width="50" title="note"></td>
<td><img src="web/images/open.png" width="50" title="open"></td>
<td><img src="web/images/ironman.png" width="50" title="ironman"></td>
<td><img src="web/images/santa.png" width="50" title="santa"></td>
<td><img src="web/images/home2.png" width="50" title="home"></td>
<td><img src="web/images/cat5.png" width="50" title="cat"></td>
</tr>
</table>


## BLE Frame Format

Each frame sent via BLE follows this structure:
The full BLE frame is always kept below the maximum MTU, ensuring that it fits in a single BLE write without fragmentation.

```
[Header: 8 bytes][Palette: N×3 bytes][Pixels: 128 bytes]
```

Header (6 bytes):

| Byte | Name | Type | Range | Description |
|------|------|------|-------|-------------|
| `[0]` | **mode** | uint8 | 0-3 | Display mode: 0=Draw, 1=Gallery, 2=Settings, 3=Clock |
| `[1]` | **brightness** | uint8 | 0-255 | Global brightness, default: 25 |
| `[2]` | **paletteSize** | uint8 | 1-16 | Number of colors in palette |
| `[3]` | **frameIndex** | uint8 | 0-255 | Multi-purpose: Current frame index (Gallery), Color index (Clock), or Next default mode (Settings) |
| `[4]` | **totalFrames** | uint8 | 1-10 | Total number of frames |
| `[5]` | **transition** | uint8 | 0-8 | Transition effect mode, 0=Progressive, 1=Instant |
| `[6-7]` | **frameDuration** | uint16 | 1-65535 | Frame duration in seconds (little-endian), default: 15 |
