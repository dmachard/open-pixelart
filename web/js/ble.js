// ble.js - Module for handling Bluetooth Low Energy (BLE) connections and data transmission

(function () {
    // Internal storage of the last-connected device/characteristic
    let _device = null;
    let _characteristic = null;

    async function connect(serviceUuid, charUuid) {
        if (!navigator.bluetooth) throw new Error('BLE not supported in this browser.');

        const device = await navigator.bluetooth.requestDevice({
            filters: [{ services: [serviceUuid] }]
        });

        const server = await device.gatt.connect();
        const service = await server.getPrimaryService(serviceUuid);
        const characteristic = await service.getCharacteristic(charUuid);

        _device = device;
        _characteristic = characteristic;

        return { device, characteristic };
    }

    async function disconnect() {
        if (_device && _device.gatt.connected) {
            await _device.gatt.disconnect();
            console.log('BLE disconnected.');
        }
        _device = null;
        _characteristic = null;
    }

    function getCharacteristic() {
        if (!_characteristic) throw new Error('BLE not connected.');
        return _characteristic;
    }

    function isConnected() {
        return this.device && this.device.gatt && this.device.gatt.connected;
    }

    function isSupported() {
        return !!navigator.bluetooth;
    }

    function getDevice() {
        return _device;
    }

    async function getDeviceInfo() {
        if (!_device) return null;
        
        try {
            // Use existing connection if available
            let server = _device.gatt;
            if (!server.connected) {
                await server.connect();
            }
            
            const service = await server.getPrimaryService('12345678-1234-1234-1234-123456789012');
            console.log('✓ Service found');
            
            // Try to read device info characteristic
            try {
                const infoChar = await service.getCharacteristic('12345678-4321-1234-4321-123456789012');
                console.log('✓ Info characteristic found');
                
                const value = await infoChar.readValue();
                console.log('✓ Read value:', value);
                
                const decoder = new TextDecoder();
                const jsonStr = decoder.decode(value);
                console.log('✓ Decoded JSON:', jsonStr);
                
                const info = JSON.parse(jsonStr);
                console.log('✓ Device info:', info);
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
    Object.assign(window.ledmatrix.ble, { connect, disconnect, getCharacteristic, isConnected, isSupported, getDevice, getDeviceInfo});
})();