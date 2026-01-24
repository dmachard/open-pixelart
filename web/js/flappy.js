(function () {
    const statusText = document.getElementById('flappyStatusText');
    const scoreText = document.getElementById('flappyScore');
    const startBtn = document.getElementById('startFlappyBtn');
    const backBtn = document.getElementById('backFromFlappy');
    const jumpBtn = document.getElementById('btnFlappyJump');

    const ROWS = 16;
    const COLS = 16;
    const FPS = 30; // High framerate for smooth movement

    // Game state
    let birdY = 8;
    let birdVelocity = 0;
    const gravity = 0.5;
    const jumpStrength = -2.5; // Negative Y is up

    let pipes = []; // {x, gapY, gapHeight}
    let frameCount = 0;

    let score = 0;
    let isActive = false;
    let gameLoopId = null;

    // Colors
    const COLOR_BIRD = 4; // Yellow
    const COLOR_PIPE = 5; // Green
    const COLOR_BG = 0;   // Black (or Sky Blue?)

    function initGame() {
        birdY = 8;
        birdVelocity = 0;
        pipes = [];
        frameCount = 0;
        score = 0;
        isActive = true;

        updateScore();
        statusText.textContent = 'Pause';

        gameLoopId = requestAnimationFrame(gameLoop);
    }

    function spawnPipe() {
        const gapHeight = 4; // Easy gap
        const gapY = Math.floor(Math.random() * (ROWS - gapHeight - 2)) + 1;
        pipes.push({
            x: COLS,
            gapY: gapY,
            gapHeight: gapHeight
        });
    }

    function gameLoop() {
        if (!isActive) return;

        // Update physics
        birdVelocity += gravity * 0.2; // Scaled gravity
        birdY += birdVelocity;

        // Pipe generation
        if (frameCount % 25 === 0) { // Every ~25 frames
            spawnPipe();
        }
        frameCount++;

        // Update pipes
        for (let i = pipes.length - 1; i >= 0; i--) {
            pipes[i].x -= 0.5; // Move speed

            // Remove off-screen pipes
            if (pipes[i].x < -1) {
                pipes.splice(i, 1);
                score++;
                updateScore();
            }
        }

        // Collision detection
        // 1. Floor/Ceiling
        if (birdY < 0 || birdY >= ROWS) {
            gameOver();
            return;
        }

        // 2. Pipes
        const birdX = 2; // Fixed X position
        const birdIntY = Math.floor(birdY);

        for (const pipe of pipes) {
            const pipeIntX = Math.floor(pipe.x);
            if (pipeIntX === birdX) {
                // Check gap verticality
                if (birdIntY < pipe.gapY || birdIntY >= pipe.gapY + pipe.gapHeight) {
                    gameOver();
                    return;
                }
            }
        }

        sendFrame(); // Send visuals

        // Loop throttle
        setTimeout(() => {
            if (isActive) gameLoopId = requestAnimationFrame(gameLoop);
        }, 1000 / FPS);
    }

    function packBoard() {
        const packed = new Uint8Array(128);
        const birdX = 2;
        const birdIntY = Math.floor(birdY);

        const getPixel = (x, y) => {
            // Draw Bird
            if (x === birdX && y === birdIntY) return COLOR_BIRD;

            // Draw Pipes
            for (const pipe of pipes) {
                if (Math.floor(pipe.x) === x) {
                    if (y < pipe.gapY || y >= pipe.gapY + pipe.gapHeight) {
                        return COLOR_PIPE;
                    }
                }
            }
            return COLOR_BG;
        };

        for (let i = 0; i < 128; i++) {
            const p1_idx = i * 2;
            const p2_idx = i * 2 + 1;

            const px1 = getPixel(p1_idx % 16, Math.floor(p1_idx / 16));
            const px2 = getPixel(p2_idx % 16, Math.floor(p2_idx / 16));

            packed[i] = ((px1 & 0x0F) << 4) | (px2 & 0x0F);
        }
        return packed;
    }

    async function sendFrame() {
        if (!isActive) return;
        const char = window.ledmatrix?.ble?.getCharacteristic?.();
        if (!char) return;

        const packedData = packBoard();
        const header = new Uint8Array(8);
        header[0] = 5; // MODE_GAME

        const packet = new Uint8Array(8 + 128);
        packet.set(header, 0);
        packet.set(packedData, 8);

        try {
            await char.writeValue(packet);
        } catch (e) { }
    }

    // Controls
    const jump = (e) => {
        if (e) e.preventDefault();
        if (!isActive) return;
        birdVelocity = jumpStrength;
    };

    jumpBtn.addEventListener('touchstart', jump, { passive: false });
    jumpBtn.addEventListener('mousedown', jump);

    function updateScore() {
        scoreText.textContent = score;
    }

    function gameOver() {
        isActive = false;
        statusText.textContent = 'GAME OVER';
    }

    function stopGame() {
        isActive = false;
        statusText.textContent = 'Resume';
        cancelAnimationFrame(gameLoopId);
    }

    const toggleGame = (e) => {
        if (e) e.preventDefault();
        if (statusText.textContent === 'GAME OVER') {
            initGame();
        } else {
            isActive ? stopGame() : initGame();
        }
    };
    startBtn.addEventListener('click', toggleGame);
    startBtn.addEventListener('touchend', toggleGame);

    backBtn.addEventListener('click', () => {
        stopGame();
        window.showPage('modePage');
    });

    window.initFlappyMode = function () {
        window.showPage('flappyPage');
    };

})();
