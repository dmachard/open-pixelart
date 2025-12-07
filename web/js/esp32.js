// Helper to convert hex color string to [r,g,b] array
function hexToRgb(hex) {
    if (Array.isArray(hex)) return hex;
    const v = hex.replace('#', '');
    return [
        parseInt(v.slice(0, 2), 16), 
        parseInt(v.slice(2, 4), 16), 
        parseInt(v.slice(4, 6), 16)
    ];
}

// Minimal ESP32 BLE helper
(function () {
    let _isWriting = false;
    let _mtu = 512; // MTU size (default 512 bytes)

    // Set MTU size for BLE communication
    function setMTU(mtu) {
        _mtu = mtu;
        console.log(`MTU configured: ${mtu} bytes`);
    }

    // Send pixel data with fragmentation if needed
    async function send({ 
        pixels, 
        palette, 
        brightness = 25, 
        mode = 0, 
        frameIndex=0,
        totalFrames=0, 
        transition=0,
        frameDuration = 15
    } = {}) {
        const char = window.ledmatrix?.ble?.getCharacteristic?.();
        if (!char) throw new Error('BLE is not connected.');
        if (_isWriting) return; // skip this update

        _isWriting = true;

        try {
            // Prepare palette (max 16 colors)
            const finalPalette = palette.slice(0, 16).map(hexToRgb);
                
            // Buffer format (bytes):
            // [0] mode (1 byte)
            // [1] brightness (1 byte)
            // [2] paletteSize (1 byte = N)
            // [3] frame index
            // [4] total frames
            // [5] transition mode (1 byte)
            // [6-7] frame duration in seconds (2 bytes, uint16_t, little-endian)
            // [8..(8+3*N-1)] palette RGB triplets (3*N bytes)
            // [...] packed pixel indices (2 per byte)
            const data = [];
            data.push(mode);
            data.push(brightness);
            data.push(finalPalette.length);
            data.push(frameIndex);
            data.push(totalFrames);
            data.push(transition);
            
            // Frame duration as uint16_t (little-endian)
            const durationClamped = Math.max(1, Math.min(frameDuration, 65535));
            data.push(durationClamped & 0xFF);        // Low byte
            data.push((durationClamped >> 8) & 0xFF); // High byte

            // Palette RGB triplets
            finalPalette.forEach(([r, g, b]) => data.push(r, g, b));

            // Pack pixels (2 per byte)
            for (let i = 0; i < pixels.length; i += 2) {
                const a = pixels[i] & 0x0F;
                const b = pixels[i + 1] & 0x0F;
                data.push((a << 4) | b);
            }

            const buffer = new Uint8Array(data);
            // check if fragmentation is needed
            const maxPayload = _mtu - 3;
            if (buffer.length <= maxPayload) {
                // direct write
                await char.writeValue(buffer);
                console.log(`Frame sent: ${buffer.length} bytes`);
            } else {
                // send fragmented
                console.log(`Sending fragmented: ${buffer.length} bytes (MTU: ${_mtu})`);
                await sendFragmented(char, buffer);
            }

            } catch (e) {
                console.error('Send error:', e);
            } finally {
                _isWriting = false;
            }
        }

    // Send fragmented data
    async function sendFragmented(char, data) {
        // 3 bytes reserved for: [0xFF][fragIndex][fragTotal]
        const fragmentPayloadSize = _mtu - 3 - 3; // -3 BLE header, -3 fragment header
        const totalFragments = Math.ceil(data.length / fragmentPayloadSize);
        
        console.log(`Fragmenting: ${data.length} bytes → ${totalFragments} fragments`);
        
        for (let i = 0; i < totalFragments; i++) {
            const start = i * fragmentPayloadSize;
            const end = Math.min(start + fragmentPayloadSize, data.length);
            const fragmentData = data.slice(start, end);
            
            // Format: [0xFF][fragIndex][fragTotal][...data...]
            const fragment = new Uint8Array(3 + fragmentData.length);
            fragment[0] = 0xFF;              // Fragment identifier
            fragment[1] = i;                 // Fragment index
            fragment[2] = totalFragments;    // Total fragments
            fragment.set(fragmentData, 3);   // Data 
            
            await char.writeValue(fragment);
            console.log(`Fragment ${i + 1}/${totalFragments} (${fragmentData.length} bytes)`);
            
            // Small delay between fragments
            if (i < totalFragments - 1) {
                await new Promise(resolve => setTimeout(resolve, 10));
            }
        }
        
        console.log(`Fragmented frame complete!`);
    }

    window.ledmatrix = window.ledmatrix || {};
    window.ledmatrix.esp32 = window.ledmatrix.esp32 || {};
    Object.assign(window.ledmatrix.esp32, { send, setMTU });
})();
