const DEBUG = new URLSearchParams(window.location.search).get("debug") === "1";

if (DEBUG) {
    console.warn("Debug mode enabled");

    window.showPage = function (pageId) {
        document.querySelectorAll(".page").forEach(p => p.classList.remove("active"));
        document.getElementById(pageId).classList.add("active");
    };

    window.debugGoto = {
        connection: () => showPage("connectionPage"),
        mode: () => showPage("modePage"),
        draw: () => showPage("drawPage"),
        slideshow: () => showPage("slideshowPage"),
    };

    setTimeout(() => showPage("modePage"), 50);

    navigator.bluetooth = {
        requestDevice: () => Promise.reject("Bluetooth disabled in debug mode"),
    };
}

let currentDevice = null;
let currentDeviceInfo = null;
window.globalBrightness = 25; // Default brightness
window.clockColorIndex = 2;   // Default color (Cyan)
window.clockGradientIndex = 0; // Default gradient (Rainbow)

const SERVICE_UUID = '12345678-1234-1234-1234-123456789012';
const CHAR_UUID = '87654321-4321-4321-4321-210987654321';

// ========== NOTIFICATIONS ==========
function showNotification(message, isError = false) {
    const notif = document.createElement('div');
    notif.className = 'notification' + (isError ? ' error' : '');
    notif.textContent = message;
    document.body.appendChild(notif);
    setTimeout(() => notif.remove(), 2000);
}


// ========== CONNECTION STATUS ==========
function showConnectionStatus(message, type = 'connecting') {
    const status = document.getElementById('connectionStatus');
    const messageEl = document.getElementById('connectionMessage');

    if (!status || !messageEl) return;

    messageEl.textContent = message;
    status.className = `connection-status show ${type}`;
}

function hideConnectionStatus() {
    const status = document.getElementById('connectionStatus');
    if (status) {
        status.classList.remove('show');
    }
}


// ========== NAVIGATION ==========
function showPage(pageId) {
    document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
    document.getElementById(pageId).classList.add('active');
}

// ========== CONNECTION PAGE ==========
document.getElementById('connectBtn').addEventListener('click', async () => {
    const btn = document.getElementById('connectBtn');

    // Disable button during connection
    btn.disabled = true;
    btn.classList.add("btn-hide");

    try {
        if (!window.ledmatrix?.ble?.isSupported?.()) {
            throw new Error('Bluetooth not supported');
        }

        // Connect to device
        showConnectionStatus('Connecting to device...', 'connecting');
        const { device } = await window.ledmatrix.ble.connect(SERVICE_UUID, CHAR_UUID);
        currentDevice = device;

        // Try to read device info
        currentDeviceInfo = await window.ledmatrix.ble.getDeviceInfo();

        const deviceLabel = currentDeviceInfo?.model || device.name || 'LED Matrix';
        showConnectionStatus('Connected successfully!', 'connecting');

        // Update device display
        const deviceNameEl = document.getElementById('deviceName');
        if (currentDeviceInfo) {
            if (currentDeviceInfo.brightness !== undefined) {
                window.globalBrightness = currentDeviceInfo.brightness;
                console.log('Restored brightness from device:', window.globalBrightness);
            }

            const dimensionsText = currentDeviceInfo.width && currentDeviceInfo.height
                ? `${currentDeviceInfo.width}×${currentDeviceInfo.height}`
                : '16×16';

            const dimensions = ` (${dimensionsText})`;
            deviceNameEl.textContent = deviceLabel + dimensions;
            deviceNameEl.title = 'Model: ' + deviceLabel + ', Dimensions: ' +
                dimensionsText + ', Brightness: ' + window.globalBrightness;

            // Update settings info
            const infoModelEl = document.getElementById('infoModel');
            const infoResEl = document.getElementById('infoResolution');
            if (infoModelEl) infoModelEl.textContent = deviceLabel;
            if (infoResEl) infoResEl.textContent = dimensionsText;

            // Update clock color if available
            if (currentDeviceInfo.clockColorIndex !== undefined) {
                window.clockColorIndex = currentDeviceInfo.clockColorIndex;
            }

            // Update clock gradient if available
            if (currentDeviceInfo.clockGradientIndex !== undefined) {
                window.clockGradientIndex = currentDeviceInfo.clockGradientIndex;
            }
        } else {
            deviceNameEl.textContent = deviceLabel;
        }

        // Transition to mode page
        setTimeout(() => {
            hideConnectionStatus();
            showPage('modePage');
        }, 500);

        // Handle disconnection
        device.addEventListener('gattserverdisconnected', () => {
            currentDeviceInfo = null;
            showNotification('Disconnected');
            showPage('connectionPage');
            hideConnectionStatus();

            // Re-enable connect button
            btn.disabled = false;
            btn.classList.remove("btn-hide");
        });
    } catch (error) {
        console.error('Connection failed:', error);

        // Display error message
        showConnectionStatus(
            `${error.message}`,
            'error'
        );

        // Hide after delay
        setTimeout(() => {
            hideConnectionStatus();
            btn.disabled = false;
            btn.classList.remove("btn-hide");
        }, 3000);
    }
});


// ========== GAME OVER BLINKER ==========
let gameOverInterval = null;

window.stopGameOver = function () {
    if (gameOverInterval) {
        clearInterval(gameOverInterval);
        gameOverInterval = null;
    }
};

window.showGameOver = function () {
    window.stopGameOver(); // Clear existing

    console.log("Starting GAME OVER blink sequence");

    // 3x5 Font Definitions (1 = pixel on)
    // Coords: [y, x] relative to char origin
    const FONT = {
        // G is 4x5
        'G': [[0, 1], [0, 2], [1, 0], [2, 0], [2, 2], [2, 3], [3, 0], [3, 3], [4, 1], [4, 2]],
        'A': [[0, 1], [1, 0], [1, 2], [2, 0], [2, 1], [2, 2], [3, 0], [3, 2], [4, 0], [4, 2]],
        'M': [[0, 0], [0, 2], [1, 0], [1, 1], [1, 2], [2, 0], [2, 2], [3, 0], [3, 2], [4, 0], [4, 2]],
        'E': [[0, 0], [0, 1], [0, 2], [1, 0], [2, 0], [2, 1], [3, 0], [4, 0], [4, 1], [4, 2]],
        // O is 4x5
        'O': [[0, 1], [0, 2], [1, 0], [1, 3], [2, 0], [2, 3], [3, 0], [3, 3], [4, 1], [4, 2]],
        'V': [[0, 0], [0, 2], [1, 0], [1, 2], [2, 0], [2, 2], [3, 1], [4, 1]], // 'V' tweaked to fit
        'R': [[0, 0], [0, 1], [1, 0], [1, 2], [2, 0], [2, 1], [3, 0], [3, 2], [4, 0], [4, 2]]
    };

    function drawChar(pixels, char, offsetX, offsetY, color) {
        const shape = FONT[char];
        if (!shape) return;
        shape.forEach(([y, x]) => {
            const py = offsetY + y;
            const px = offsetX + x;
            if (px >= 0 && px < 16 && py >= 0 && py < 16) {
                const idx = (py * 16 + px);
                pixels[idx] = color; // Palette index
            }
        });
    }

    const colorRed = 7; // Palette index for Red

    // Construct the "GAME OVER" frame
    // Line 1: GAME (y=2)
    // Line 2: OVER (y=9)
    // Spacing: 3px char + 1px gap. Total 4 chars = 15px width.
    // Start X = 0 or 1 to center? 15px is odd, 16px is even. Start at 0 with 4px gap logic?
    // G(0-2) gap(3) A(4-6) gap(7) M(8-10) gap(11) E(12-14) -> Width 15. Center at 0? 
    // Let's use startX=0.

    const frameOn = new Uint8Array(256).fill(0);

    // Draw GAME
    drawChar(frameOn, 'G', 0, 2, colorRed); // x:0..3 (width 4)
    drawChar(frameOn, 'A', 5, 2, colorRed); // x:5..7 (width 3)
    drawChar(frameOn, 'M', 9, 2, colorRed); // x:9..11 (width 3)
    drawChar(frameOn, 'E', 13, 2, colorRed); // x:13..15 (width 3)

    // Draw OVER
    drawChar(frameOn, 'O', 0, 9, colorRed); // x:0..3 (width 4)
    drawChar(frameOn, 'V', 5, 9, colorRed); // x:5..7 (width 3)
    drawChar(frameOn, 'E', 9, 9, colorRed); // x:9..11 (width 3)
    drawChar(frameOn, 'R', 13, 9, colorRed); // x:13..15 (width 3)

    const frameOff = new Uint8Array(256).fill(0);
    let isOn = true;

    // Helper to send raw frame
    const sendFrame = async (pixels) => {
        try {
            // Pack 2 pixels per byte
            const packed = new Uint8Array(128);
            for (let i = 0; i < 128; i++) {
                const p1 = pixels[i * 2] & 0x0F;
                const p2 = pixels[i * 2 + 1] & 0x0F;
                packed[i] = (p1 << 4) | p2;
            }

            const char = window.ledmatrix?.ble?.getCharacteristic?.();
            if (!char) return;

            // Use Mode 5 (Game) or generic raw
            const header = new Uint8Array(8);
            header[0] = 5; // Mode Game

            const packet = new Uint8Array(8 + 128);
            packet.set(header, 0);
            packet.set(packed, 8);

            await char.writeValue(packet);
        } catch (e) {
            console.error("Blink error", e);
        }
    };

    // Initial draw
    sendFrame(frameOn);

    gameOverInterval = setInterval(() => {
        isOn = !isOn;
        sendFrame(isOn ? frameOn : frameOff);
    }, 600); // Blink every 600ms
};

// ========== TEXT HELPER ==========
window.sendTextToDevice = async function (text, { color = "#FFFFFF", speed = 100, effect = 0 } = {}) {
    if (!text) return;

    // Use global hexToRgb from esp32.js if available
    const rgb = typeof hexToRgb === 'function' ? hexToRgb(color) : [255, 255, 255];

    try {
        // Protocol: [MODE_TEXT (6), Brightness, R, G, B, Speed, Effect, ...ASCII]
        const payload = [
            6, // MODE_TEXT
            window.globalBrightness || 25,
            rgb[0], rgb[1], rgb[2],
            speed,
            effect
        ];

        // Append text characters
        for (let i = 0; i < text.length; i++) {
            payload.push(text.charCodeAt(i));
        }

        // Send 
        const char = window.ledmatrix?.ble?.getCharacteristic?.();
        if (char) {
            await char.writeValue(new Uint8Array(payload));
            console.log(`Text sent to device: "${text}"`);
        } else {
            console.warn("BLE not connected, cannot send text");
        }
    } catch (err) {
        console.error('Error sending text:', err);
    }
};

// ========== MODE SELECTION ==========
// ========== MODE SELECTION ==========
const modeGrid = document.querySelector('.mode-grid');
if (modeGrid) {
    modeGrid.addEventListener('click', (e) => {
        const card = e.target.closest('.mode-card');
        if (!card) return;

        const mode = card.dataset.mode;

        if (mode === 'disconnect') {
            if (currentDevice && currentDevice.gatt.connected) {
                currentDevice.gatt.disconnect();
            }

            // re-enable connect button
            const btn = document.getElementById('connectBtn');
            btn.disabled = false;
            btn.classList.remove("btn-hide");

            showPage('connectionPage');
            return;
        }

        if (mode === 'draw') {
            initDrawMode();
            showPage('drawPage');
        } else if (mode === 'gallery') {
            initSlideshowMode();
            showPage('galleryPage');
        } else if (mode === 'settings') {
            initSettingsMode();
            showPage('settingsPage');
        } else if (mode === 'clock') {
            initClockMode();
            showPage('modePage'); // No dedicated page, just send command
        } else if (mode === 'audio') {
            initAudioMode();
        } else if (mode === 'game') {
            initTetrisMode();
        } else if (mode === 'snake') {
            initSnakeMode();
        } else if (mode === 'simon') {
            initSimonMode();
        } else if (mode === 'text') {
            initTextMode();
            showPage('textPage');
        }
    });
}

// ========== SETTINGS MODE ==========
// ========== SETTINGS MODE ==========
function initSettingsMode() {
    const brightnessSelect = document.getElementById('brightnessSettingsSelect');
    const backBtn = document.getElementById('backFromSettings');
    const defaultModeSelect = document.getElementById('defaultModeSettingsSelect');
    const clockColorSelect = document.getElementById('clockColorSettingsSelect');
    const clockGradientSelect = document.getElementById('clockGradientSettingsSelect');

    // Helper to replace element to strip listeners
    const replaceElement = (el) => {
        if (!el) return null;
        const newEl = el.cloneNode(true);
        el.parentNode.replaceChild(newEl, el);
        return newEl;
    };

    // 1. Back Button
    const newBackBtn = replaceElement(backBtn);
    if (newBackBtn) {
        newBackBtn.addEventListener('click', () => showPage('modePage'));
    }

    // 2. Brightness
    const newBrightnessSelect = replaceElement(brightnessSelect);
    if (newBrightnessSelect) {
        newBrightnessSelect.value = window.globalBrightness;
        newBrightnessSelect.addEventListener('change', async (e) => {
            window.globalBrightness = parseInt(e.target.value);
            console.log('Changing global brightness to:', window.globalBrightness);

            try {
                // Send a dummy frame with MODE_SETTINGS (2)
                await window.ledmatrix.esp32.send({
                    pixels: new Array(16 * 16).fill(0),
                    palette: ['#000000'],
                    brightness: window.globalBrightness,
                    mode: 2, // MODE_SETTINGS
                    frameIndex: 255, // 255 = no change to default mode
                    totalFrames: 1
                });
                showNotification('✓ Brightness saved');
            } catch (err) {
                console.error('Error saving brightness:', err);
                showNotification('✗ Error saving brightness', true);
            }
        });
    }

    // 3. Default Mode
    const newDefaultModeSelect = replaceElement(defaultModeSelect);
    if (newDefaultModeSelect) {
        newDefaultModeSelect.addEventListener('change', async (e) => {
            const newDefaultMode = parseInt(e.target.value);
            console.log('Changing default mode to:', newDefaultMode);

            try {
                await window.ledmatrix.esp32.send({
                    pixels: new Array(16 * 16).fill(0),
                    palette: ['#000000'],
                    brightness: window.globalBrightness,
                    mode: 2, // MODE_SETTINGS
                    frameIndex: newDefaultMode,
                    totalFrames: 1
                });
                showNotification('✓ Default mode saved');
            } catch (err) {
                console.error('Error saving default mode:', err);
                showNotification('✗ Error saving default mode', true);
            }
        });
    }

    // 4. Clock Color
    const newClockColorSelect = replaceElement(clockColorSelect);
    if (newClockColorSelect) {
        newClockColorSelect.value = window.clockColorIndex;
        newClockColorSelect.addEventListener('change', async (e) => {
            window.clockColorIndex = parseInt(e.target.value);
            console.log('Changing clock color to:', window.clockColorIndex);

            try {
                await window.ledmatrix.esp32.send({
                    pixels: new Array(16 * 16).fill(0),
                    palette: ['#000000'],
                    brightness: window.globalBrightness,
                    mode: 3, // MODE_CLOCK
                    frameIndex: window.clockColorIndex,
                    totalFrames: window.clockGradientIndex
                });
                showNotification('✓ Clock color saved');
            } catch (err) {
                console.error('Error saving clock color:', err);
                showNotification('✗ Error saving clock color', true);
            }
        });
    }

    // 5. Clock Gradient
    const newClockGradientSelect = replaceElement(clockGradientSelect);
    if (newClockGradientSelect) {
        newClockGradientSelect.value = window.clockGradientIndex;
        newClockGradientSelect.addEventListener('change', async (e) => {
            window.clockGradientIndex = parseInt(e.target.value);
            console.log('Changing clock gradient to:', window.clockGradientIndex);

            try {
                await window.ledmatrix.esp32.send({
                    pixels: new Array(16 * 16).fill(0),
                    palette: ['#000000'],
                    brightness: window.globalBrightness,
                    mode: 3, // MODE_CLOCK
                    frameIndex: window.clockColorIndex,
                    totalFrames: window.clockGradientIndex
                });
                showNotification('✓ Clock gradient saved');
            } catch (err) {
                console.error('Error saving clock gradient:', err);
                showNotification('✗ Error saving clock gradient', true);
            }
        });
    }
}

// ========== CLOCK MODE ==========
async function initClockMode() {
    console.log('Switching to Clock Mode');
    try {
        await window.ledmatrix.esp32.send({
            pixels: new Array(16 * 16).fill(0),
            palette: ['#000000'],
            brightness: window.globalBrightness,
            mode: 3, // MODE_CLOCK
            frameIndex: window.clockColorIndex,
            totalFrames: window.clockGradientIndex,
            frameDuration: 0
        });
        showNotification('🕒 Clock mode activated');
    } catch (err) {
        console.error('Error switching to clock mode:', err);
        showNotification('✗ Error clock mode', true);
    }
}

// ========== TEXT MODE ==========

