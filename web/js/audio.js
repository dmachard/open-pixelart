(function () {
    let audioContext = null;
    let analyser = null;
    let dataArray = null;
    let source = null;
    let isActive = false;
    let animationId = null;

    // UI Elements
    // UI Elements
    // Canvas removed as requested
    const toggleBtn = document.getElementById('toggleAudioBtn');
    const statusText = document.getElementById('audioStatusText');
    const backBtn = document.getElementById('backFromAudio');

    async function startAudio() {
        if (isActive) return;

        try {
            const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
            audioContext = new (window.AudioContext || window.webkitAudioContext)();
            analyser = audioContext.createAnalyser();
            source = audioContext.createMediaStreamSource(stream);
            source.connect(analyser);

            analyser.fftSize = 256;
            const bufferLength = analyser.frequencyBinCount;
            dataArray = new Uint8Array(bufferLength);

            isActive = true;
            statusText.textContent = 'Stop';
            toggleBtn.querySelector('.icon').textContent = '⏹️';

            // Start sending data to ESP32
            sendToESP32();

            console.log('Audio visualizer started');
        } catch (err) {
            console.error('Error accessing microphone:', err);
            alert('Could not access microphone. Please check permissions.');
        }
    }

    function stopAudio() {
        if (!isActive) return;

        isActive = false;
        statusText.textContent = 'Start';
        toggleBtn.querySelector('.icon').textContent = '▶️';

        if (animationId) cancelAnimationFrame(animationId);
        if (source) source.disconnect();
        if (audioContext) audioContext.close();

        console.log('Audio visualizer stopped');
    }

    function calculateSpectrum(inputData) {
        const spectrum = new Uint8Array(16);
        // We want to focus on 0-14kHz roughly.
        // FFT size 256 -> 128 bins. Sample rate ~44-48k.
        // 128 bins over 22k -> ~172Hz per bin.
        // To cover ~14kHz, we need ~81 bins.
        // Let's use 5 bins per bar * 16 bars = 80 bins.
        const binsPerBar = 5;
        const gain = 2.0; // Boost the signal

        for (let i = 0; i < 16; i++) {
            let sum = 0;
            for (let j = 0; j < binsPerBar; j++) {
                // Offset i * binsPerBar
                const binIndex = i * binsPerBar + j;
                if (binIndex < inputData.length) {
                    sum += inputData[binIndex];
                }
            }
            // Average
            let value = sum / binsPerBar;

            // Apply gain
            value = value * gain;

            // Clamp
            if (value > 255) value = 255;

            spectrum[i] = value;
        }
        return spectrum;
    }

    let lastSend = 0;
    async function sendToESP32() {
        if (!isActive) return;

        const now = Date.now();
        // Limit sending frequency to ~20FPS (50ms)
        if (now - lastSend < 50) {
            setTimeout(sendToESP32, 10);
            return;
        }
        lastSend = now;

        analyser.getByteFrequencyData(dataArray);

        const spectrum = calculateSpectrum(dataArray);
        const mappedSpectrum = new Uint8Array(16);

        for (let i = 0; i < 16; i++) {
            // Map 0-255 to 0-16 for the display
            mappedSpectrum[i] = Math.round((spectrum[i] / 255) * 16);
        }

        try {
            const styleSelect = document.getElementById('audioStyleSelect');
            const style = styleSelect ? parseInt(styleSelect.value) : 0;
            await sendAudioSpectrum(mappedSpectrum, style);
        } catch (e) {
            console.error('BLE send error:', e);
        }

        setTimeout(sendToESP32, 30);
    }

    async function sendAudioSpectrum(spectrum, style) {
        const char = window.ledmatrix?.ble?.getCharacteristic?.();
        if (!char) return;

        // Header: [mode, brightness, style, frameIndex, totalFrames, transition, durL, durH]
        // Mode 4 = AUDIO
        // data[2] = style
        const header = [4, window.globalBrightness || 25, style || 0, 0, 0, 0, 0, 0];
        const data = new Uint8Array(header.length + spectrum.length);
        data.set(header, 0);
        data.set(spectrum, header.length);

        // Direct write without fragmentation since it's only 24 bytes
        try {
            await char.writeValue(data);
        } catch (e) {
            // Ignore errors if we are sending too fast
        }
    }

    toggleBtn?.addEventListener('click', () => {
        if (isActive) stopAudio();
        else startAudio();
    });

    backBtn?.addEventListener('click', () => {
        stopAudio();
        window.showPage('modePage');
    });

    window.initAudioMode = function () {
        window.showPage('audioPage');
    };

})();
