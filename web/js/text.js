function initTextMode() {
    console.log("Initializing Text Mode UI");

    const sendBtn = document.getElementById('sendTextBtn');
    const backBtn = document.getElementById('backFromText');
    const colorPicker = document.getElementById('textColorPicker');
    const colorValue = document.getElementById('textColorValue');
    const textInput = document.getElementById('textInput');
    const speedInput = document.getElementById('textSpeedInput');
    const speedValue = document.getElementById('textSpeedValue');
    const effectSelect = document.getElementById('textEffectSelect');

    // Back Navigation
    if (backBtn) {
        // Remove old listener to avoid duplicates if any (though usually we replace the node or just set onclick)
        // Using onclick is safer for simple single-listener replacement without cloning
        backBtn.onclick = () => showPage('modePage');
    }

    // Color Picker UI
    if (colorPicker && colorValue) {
        colorPicker.oninput = (e) => {
            colorValue.textContent = e.target.value.toUpperCase();
        };
    }

    // Speed Slider UI
    if (speedInput && speedValue) {
        speedInput.oninput = (e) => {
            const val = parseInt(e.target.value);
            if (val < 50) speedValue.textContent = "Very Fast";
            else if (val < 80) speedValue.textContent = "Fast";
            else if (val < 120) speedValue.textContent = "Medium";
            else if (val < 180) speedValue.textContent = "Slow";
            else speedValue.textContent = "Very Slow";
        };
    }

    // Send Button Logic
    if (sendBtn) {
        sendBtn.onclick = async () => {
            const text = textInput.value.toUpperCase();
            if (!text) return showNotification('Please enter text', true);

            const colorHex = colorPicker.value;
            const speed = parseInt(speedInput.value);
            // Default to 0 if element missing or value invalid
            const effect = parseInt(effectSelect ? effectSelect.value : 0);

            console.log(`Sending text: "${text}", Speed: ${speed}, Effect: ${effect}`);

            const rgb = hexToRgb(colorHex);

            try {
                // Protocol: [MODE_TEXT (6), Brightness, R, G, B, Speed, Effect, ...ASCII]
                const payload = [
                    6, // MODE_TEXT
                    window.globalBrightness,
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
                    showNotification('✓ Message sent');
                } else {
                    throw new Error("BLE not connected");
                }
            } catch (err) {
                console.error('Error sending text:', err);
                showNotification('✗ Failed to send', true);
            }
        };
    }
}
