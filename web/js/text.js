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

            await window.sendTextToDevice(text, {
                color: colorHex,
                speed: speed,
                effect: effect
            });

            showNotification('✓ Message sent');
        };
    }
}
