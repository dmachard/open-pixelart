// ble.js - Module for handling Bluetooth Low Energy (BLE) connections and data transmission

(function () {
    // Internal storage of the last-connected device/characteristic
    let _device = null;
    let _server = null;
    let _service = null;
    let _characteristic = null;

    async function connect(serviceUuid, charUuid) {
        if (!navigator.bluetooth) throw new Error('BLE not supported in this browser.');

        const device = await navigator.bluetooth.requestDevice({
            filters: [
                { services: [serviceUuid] }
            ],
            optionalServices: [serviceUuid] // Recommended for Android
        });

        const server = await device.gatt.connect();
        const service = await server.getPrimaryService(serviceUuid);
        const characteristic = await service.getCharacteristic(charUuid);

        _device = device;
        _server = server;
        _service = service;
        _characteristic = characteristic;

        // Attempt to read device info
        // We wait a bit on Android to allow the GATT stack to stabilize
        await new Promise(r => setTimeout(r, 500));

        try {
            const info = await getDeviceInfo();
            if (info && info.mtu) {
                if (window.ledmatrix?.esp32?.setMTU) {
                    window.ledmatrix.esp32.setMTU(info.mtu);
                }
            }
        } catch (e) {
            console.warn('Could not retrieve device info:', e);
        }

        return { device, characteristic };
    }

    async function disconnect() {
        if (_device && _device.gatt.connected) {
            await _device.gatt.disconnect();
            console.log('BLE disconnected.');
        }
        _device = null;
        _server = null;
        _service = null;
        _characteristic = null;
    }

    function getCharacteristic() {
        if (!_characteristic) throw new Error('BLE not connected.');
        return _characteristic;
    }

    function isConnected() {
        return _device && _device.gatt && _device.gatt.connected;
    }

    function isSupported() {
        return !!navigator.bluetooth;
    }

    function getDevice() {
        return _device;
    }

    async function getDeviceInfo() {
        if (!_device || !_service) return null;

        try {
            // Use cached service or fetch it if missing
            let service = _service;
            if (!_service) {
                if (!_server.connected) await _server.connect();
                service = await _server.getPrimaryService('12345678-1234-1234-1234-123456789012');
            }

            // Info characteristic UUID (defined in ESP32 config.h)
            const INFO_CHAR_UUID = '12345678-4321-1234-4321-123456789012';

            try {
                const infoChar = await service.getCharacteristic(INFO_CHAR_UUID);
                const value = await infoChar.readValue();

                const decoder = new TextDecoder();
                const jsonStr = decoder.decode(value);
                const info = JSON.parse(jsonStr);

                console.log('✓ Device info synced:', info);
                return info;
            } catch (e) {
                console.warn('Device info characteristic not available:', e.message);
                return null;
            }
        } catch (error) {
            console.error('Error reading device info:', error.message);
            return null;
        }
    }

    window.ledmatrix = window.ledmatrix || {};
    window.ledmatrix.ble = window.ledmatrix.ble || {};
    Object.assign(window.ledmatrix.ble, {
        connect,
        disconnect,
        getCharacteristic,
        isConnected,
        isSupported,
        getDevice,
        getDeviceInfo
    });
})();