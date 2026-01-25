(function () {
    const statusText = document.getElementById('simonStatusText');
    const scoreText = document.getElementById('simonScore');
    const startBtn = document.getElementById('startSimonBtn');
    const backBtn = document.getElementById('backFromSimon');

    const btnGreen = document.getElementById('btnSimonGreen');
    const btnRed = document.getElementById('btnSimonRed');
    const btnYellow = document.getElementById('btnSimonYellow');
    const btnBlue = document.getElementById('btnSimonBlue');

    const COLORS = [
        { id: 0, btn: btnGreen, colorIdx: 5 },  // Green
        { id: 1, btn: btnRed, colorIdx: 7 },    // Red
        { id: 2, btn: btnYellow, colorIdx: 4 }, // Yellow
        { id: 3, btn: btnBlue, colorIdx: 2 }    // Blue
    ];

    let sequence = [];
    let playbackIdx = 0;
    let inputIdx = 0;

    let isPlayingSequence = false;
    let isActive = false;

    // Matrix quadrant mapping
    // TL: Green, TR: Red, BL: Yellow, BR: Blue

    function initGame() {
        window?.stopGameOver?.();
        sequence = [];
        isActive = true;
        statusText.textContent = 'Watch...';
        nextLevel();
    }

    function nextLevel() {
        // Add random color
        sequence.push(Math.floor(Math.random() * 4));
        scoreText.textContent = sequence.length;

        inputIdx = 0;
        playbackIdx = 0;
        isPlayingSequence = true;

        setTimeout(playSequenceStep, 1000);
    }

    function playSequenceStep() {
        if (!isActive) return;

        if (playbackIdx >= sequence.length) {
            isPlayingSequence = false;
            statusText.textContent = 'Your Turn';
            sendFrame(-1); // Clear
            return;
        }

        const colorId = sequence[playbackIdx];
        highlightQuadrant(colorId);
        playbackIdx++;

        setTimeout(() => {
            sendFrame(-1); // Off
            setTimeout(playSequenceStep, 200); // Gap
        }, 600); // Duration of flash
    }

    function handleInput(colorId) {
        if (!isActive || isPlayingSequence) return;

        highlightQuadrant(colorId);
        setTimeout(() => sendFrame(-1), 200); // Short flash for feedback

        if (colorId === sequence[inputIdx]) {
            inputIdx++;
            if (inputIdx >= sequence.length) {
                statusText.textContent = 'Good!';
                setTimeout(nextLevel, 1000);
            }
        } else {
            gameOver();
        }
    }

    function highlightQuadrant(id) {
        sendFrame(id);
    }

    function packBoard(activeId) {
        const packed = new Uint8Array(128);
        if (activeId === -1) return packed; // All black

        const targetColor = COLORS[activeId].colorIdx;

        // Fill corresponding quadrant
        // Green: x<8, y<8
        // Red: x>=8, y<8
        // Yellow: x<8, y>=8
        // Blue: x>=8, y>=8

        for (let y = 0; y < 16; y++) {
            for (let x = 0; x < 16; x++) {
                let match = false;
                if (activeId === 0 && x < 8 && y < 8) match = true;
                if (activeId === 1 && x >= 8 && y < 8) match = true;
                if (activeId === 2 && x < 8 && y >= 8) match = true;
                if (activeId === 3 && x >= 8 && y >= 8) match = true;

                if (match) {
                    const i = y * 16 + x;
                    const byteIdx = Math.floor(i / 2);
                    const isHigh = (i % 2 === 0);

                    if (isHigh) {
                        packed[byteIdx] |= (targetColor << 4);
                    } else {
                        packed[byteIdx] |= targetColor;
                    }
                }
            }
        }
        return packed;
    }

    async function sendFrame(activeId) {
        if (!isActive) return;
        const char = window.ledmatrix?.ble?.getCharacteristic?.();
        if (!char) return;

        const packet = new Uint8Array(8 + 128);
        packet[0] = 5; // Header
        packet.set(packBoard(activeId), 8);

        try { await char.writeValue(packet); } catch (e) { }
    }

    // Input handlers
    COLORS.forEach(c => {
        const handler = (e) => {
            e.preventDefault();
            handleInput(c.id);
        };
        c.btn.addEventListener('touchstart', handler);
        c.btn.addEventListener('mousedown', handler);
    });

    function gameOver() {
        isActive = false;
        statusText.textContent = 'GAME OVER';
        window?.showGameOver?.();
    }

    function stopGame() {
        window?.stopGameOver?.();
        isActive = false;
        statusText.textContent = 'Start';
    }

    startBtn.addEventListener('click', () => {
        if (statusText.textContent === 'GAME OVER') {
            initGame();
        } else {
            isActive ? stopGame() : initGame();
        }
    });

    backBtn.addEventListener('click', () => {
        stopGame();
        window.showPage('modePage');
    });

    window.initSimonMode = function () {
        window.showPage('simonPage');
    };

})();
