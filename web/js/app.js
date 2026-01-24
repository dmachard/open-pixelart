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

// ========== MODE SELECTION ==========
document.querySelectorAll('.mode-card').forEach(card => {
    card.addEventListener('click', () => {
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
        } else if (mode === 'flappy') {
            initFlappyMode();
        } else if (mode === 'arkanoid') {
            initArkanoidMode();
        }
    });
});

// ========== SETTINGS MODE ==========
function initSettingsMode() {
    const brightnessSelect = document.getElementById('brightnessSettingsSelect');
    const backBtn = document.getElementById('backFromSettings');

    // Remove existing listeners to avoid duplicates
    const newBrightnessSelect = brightnessSelect.cloneNode(true);
    brightnessSelect.parentNode.replaceChild(newBrightnessSelect, brightnessSelect);

    const newBackBtn = backBtn.cloneNode(true);
    backBtn.parentNode.replaceChild(newBackBtn, backBtn);

    newBackBtn.addEventListener('click', () => showPage('modePage'));

    newBrightnessSelect.value = window.globalBrightness;

    newBrightnessSelect.addEventListener('change', async (e) => {
        window.globalBrightness = parseInt(e.target.value);
        console.log('Changing global brightness to:', window.globalBrightness);

        try {
            // Send a dummy frame with MODE_SETTINGS (2)
            // frameIndex is 255 to indicate no change to default_mode
            await window.ledmatrix.esp32.send({
                pixels: new Array(16 * 16).fill(0),
                palette: ['#000000'],
                brightness: window.globalBrightness,
                mode: 2, // MODE_SETTINGS
                frameIndex: 255,
                totalFrames: 1
            });
            showNotification('✓ Brightness saved');
        } catch (err) {
            console.error('Error saving brightness:', err);
            showNotification('✗ Error saving brightness', true);
        }
    });

    const defaultModeSelect = document.getElementById('defaultModeSettingsSelect');
    if (defaultModeSelect) {
        // We don't have the current defaultMode from device info yet, 
        // but we can set it via MODE_SETTINGS.

        defaultModeSelect.addEventListener('change', async (e) => {
            const newDefaultMode = parseInt(e.target.value);
            console.log('Changing default mode to:', newDefaultMode);

            try {
                // Send MODE_SETTINGS (2) with default_mode in frameIndex (byte 3)
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

    const clockColorSelect = document.getElementById('clockColorSettingsSelect');
    if (clockColorSelect) {
        clockColorSelect.value = window.clockColorIndex;
        clockColorSelect.addEventListener('change', async (e) => {
            window.clockColorIndex = parseInt(e.target.value);
            console.log('Changing clock color to:', window.clockColorIndex);

            try {
                // Send MODE_CLOCK (3) with clock color index in frameIndex
                await window.ledmatrix.esp32.send({
                    pixels: new Array(16 * 16).fill(0),
                    palette: ['#000000'],
                    brightness: window.globalBrightness,
                    mode: 3, // MODE_CLOCK
                    frameIndex: window.clockColorIndex,
                    totalFrames: 1
                });
                showNotification('✓ Clock color saved');
            } catch (err) {
                console.error('Error saving clock color:', err);
                showNotification('✗ Error saving clock color', true);
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
            totalFrames: 1
        });
        showNotification('🕒 Clock mode activated');
    } catch (err) {
        console.error('Error switching to clock mode:', err);
        showNotification('✗ Error clock mode', true);
    }
}
