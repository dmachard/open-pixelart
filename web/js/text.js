function initTextMode() {
    console.log("Initializing Text Mode UI");

    const sendBtn = document.getElementById('sendTextBtn');
    const backBtn = document.getElementById('backFromText');
    const colorPicker = document.getElementById('textColorPicker');
    const colorValue = document.getElementById('textColorValue');
    const textInput = document.getElementById('textInput');
    const speedInput = document.getElementById('textSpeedInput');
    const speedValue = document.getElementById('textSpeedValue');

    // Back Navigation
    backBtn.onclick = () => showPage('modePage');

    // Color Picker UI
    colorPicker.oninput = (e) => {
        colorValue.textContent = e.target.value.toUpperCase();
    };

    // Speed Slider UI - Invert logic for display (High Delay = Slow, Low Delay = Fast)
    // Range is 20 (Fast) to 250 (Slow)
    speedInput.oninput = (e) => {
        const val = parseInt(e.target.value);
        if (val < 50) speedValue.textContent = "Very Fast";
        else if (val < 80) speedValue.textContent = "Fast";
        else if (val < 120) speedValue.textContent = "Medium";
        else if (val < 180) speedValue.textContent = "Slow";
        else speedValue.textContent = "Very Slow";
    };

    // Send Button Logic
    sendBtn.onclick = async () => {
        const text = textInput.value.toUpperCase(); // Font is uppercase only mostly
        const colorHex = colorPicker.value;
        const speed = parseInt(speedInput.value); // Delay in ms

        if (!text) {
            showNotification("Please enter some text", true);
            return;
        }

        const rgb = hexToRgb(colorHex);

        // Protocol: [MODE_TEXT (6), Brightness, R, G, B, Speed, ...ASCII]
        const header = [
            6, // MODE_TEXT
            window.globalBrightness,
            rgb[0], rgb[1], rgb[2],
            speed
        ];

        // Convert string to bytes
        const textBytes = new TextEncoder().encode(text);

        // Combine
        const payload = new Uint8Array(header.length + textBytes.length);
        payload.set(header);
        payload.set(textBytes, header.length);

        try {
            const char = window.ledmatrix.ble.getCharacteristic();
            if (!char) throw new Error("Not connected");

            await char.writeValue(payload);
            showNotification("✓ Message sent");
        } catch (err) {
            console.error(err);
            showNotification("✗ Failed to send", true);
        }
    };
}
