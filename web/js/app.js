const DEBUG = new URLSearchParams(window.location.search).get("debug") === "1";

if (DEBUG) {
    console.warn("Debug mode enabled");

    window.showPage = function(pageId) {
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

const SERVICE_UUID = '12345678-1234-1234-1234-123456789012';
const CHAR_UUID   = '87654321-4321-4321-4321-210987654321';

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
    btn.classList.add('loading');
    showConnectionStatus('🔍 Searching for devices...', 'connecting');
    
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
            const dimensions = currentDeviceInfo.width && currentDeviceInfo.height 
                ? ` (${currentDeviceInfo.width}x${currentDeviceInfo.height})` 
                : '';
            deviceNameEl.textContent = deviceLabel + dimensions;
            deviceNameEl.title = 'Model: ' + deviceLabel + ', Dimensions: ' + 
                                currentDeviceInfo.width + '×' + currentDeviceInfo.height;
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
        }, 3000);
    } finally {
        // Re-enable button
        btn.disabled = false;
        btn.classList.remove('loading');
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
            showPage('connectionPage');
            return;
        }
        
        if (mode === 'draw') {
            initDrawMode();
            showPage('drawPage');
        } else if (mode === 'gallery') {
            initSlideshowMode();
            showPage('galleryPage');
        } else if (mode === 'clock') {
            initClockMode();
            showPage('clockPage');
        }
    });
});
