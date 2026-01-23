(function () {
    let audioContext = null;
    let analyser = null;
    let dataArray = null;
    let source = null;
    let isActive = false;
    let animationId = null;

    // UI Elements
    const canvas = document.getElementById('audioCanvas');
    const canvasCtx = canvas?.getContext('2d');
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

            visualize();
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

    function visualize() {
        if (!isActive) return;
        animationId = requestAnimationFrame(visualize);

        analyser.getByteFrequencyData(dataArray);

        if (!canvasCtx) return;

        // Draw preview on canvas
        canvasCtx.fillStyle = '#1a1a1a';
        canvasCtx.fillRect(0, 0, canvas.width, canvas.height);

        const barWidth = (canvas.width / 16);
        let barHeight;
        let x = 0;

        for (let i = 0; i < 16; i++) {
            // Simple mapping: average multiple bins for each bar
            const binRange = Math.floor(dataArray.length / 16);
            let sum = 0;
            for (let j = 0; j < binRange; j++) {
                sum += dataArray[i * binRange + j];
            }
            const value = sum / binRange;

            barHeight = (value / 255) * canvas.height;

            canvasCtx.fillStyle = `hsl(${(i / 16) * 360}, 70%, 50%)`;
            canvasCtx.fillRect(x, canvas.height - barHeight, barWidth - 1, barHeight);

            x += barWidth;
        }
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

        const spectrum = new Uint8Array(16);
        const binRange = Math.floor(dataArray.length / 16);

        for (let i = 0; i < 16; i++) {
            let sum = 0;
            for (let j = 0; j < binRange; j++) {
                sum += dataArray[i * binRange + j];
            }
            const value = sum / binRange;
            // Map 0-255 to 0-16
            spectrum[i] = Math.round((value / 255) * 16);
        }

        try {
            // We need a way to send raw data. 
            // Standard esp32.send might be too heavy.
            // Let's use a specialized function or just use send with mode 4.
            await sendAudioSpectrum(spectrum);
        } catch (e) {
            console.error('BLE send error:', e);
        }

        setTimeout(sendToESP32, 30);
    }

    async function sendAudioSpectrum(spectrum) {
        const char = window.ledmatrix?.ble?.getCharacteristic?.();
        if (!char) return;

        // Header: [mode, brightness, paletteSize, frameIndex, totalFrames, transition, durL, durH]
        // Mode 4 = AUDIO
        const header = [4, window.globalBrightness || 25, 0, 0, 0, 0, 0, 0];
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
