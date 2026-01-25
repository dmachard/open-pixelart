# BLE Frame Format

**Advertised Bluetooth device name:** `PixelBox`

The device receives data via the `CHAR_DATA_UUID` characteristic.
The protocol supports fragmented frames for large payloads (marker `0xFF`).

## Standard Header (8 bytes)

Used by Drawing, Gallery, Settings, Clock.

| Byte | Name | Description |
|------|------|-------------|
| `0` | **Mode** | `0`=Draw, `1`=Gallery, `2`=Settings, `3`=Clock |
| `1` | **Brightness** | Global brightness (0-255) |
| `2` | **Palette Size** | Number of colors following the header (max 16) |
| `3` | **Frame Index** | Current frame index or Parameter |
| `4` | **Total Frames** | Total number of frames in sequence |
| `5` | **Reserved** | Reserved for future use |
| `6-7` | **Duration** | Frame duration in seconds (uint16 little-endian) |

**Payload:**
1. **Palette**: `Palette Size * 3` bytes (R, G, B for each color).
2. **Pixels**: `128` bytes (16x16 pixels, 4 bits per pixel, packed 2 pixels per byte).

**Mode Specifics:**
*   **Settings (Mode 2)**: `Frame Index` sets the default boot mode.
*   **Clock (Mode 3)**: `Frame Index` sets the clock digit color index.

## Audio Mode (Mode 4)

| Byte | Name | Description |
|------|------|-------------|
| `0` | **Mode** | `4` (Audio) |
| `1` | **Brightness** | Global brightness |
| `2` | **Style** | Audio Visualizer Style ID |
| `3-7`| **Reserved** | Header padding |
| `8-23`| **Spectrum** | 16 bytes of audio spectrum data |

## Game Mode (Mode 5)

| Byte | Name | Description |
|------|------|-------------|
| `0` | **Mode** | `5` (Game) |
| `1-7`| **Reserved** | Header padding |
| `8-135`| **Game Data** | 128 bytes of raw pixel data (packed) |

## Text Mode (Mode 6)

**Note:** Text mode uses a custom compact format.

| Byte | Name | Description |
|------|------|-------------|
| `0` | **Mode** | `6` (Text) |
| `1` | **Brightness** | Global brightness |
| `2-4`| **Color** | R, G, B color for the text |
| `5` | **Speed** | Scroll speed |
| `6...`| **Message** | ASCII text string (max 128 chars) |
