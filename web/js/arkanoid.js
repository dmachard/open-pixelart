(function () {
    const statusText = document.getElementById('arkanoidStatusText');
    const scoreText = document.getElementById('arkanoidScore');
    const startBtn = document.getElementById('startArkanoidBtn');
    const backBtn = document.getElementById('backFromArkanoid');

    const btnLeft = document.getElementById('btnArkanoidLeft');
    const btnRight = document.getElementById('btnArkanoidRight');

    const ROWS = 16;
    const COLS = 16;
    const FPS = 10;

    // Game state
    let paddleX = 6;
    const paddleWidth = 4;

    let ball = { x: 8, y: 14, dx: 1, dy: -1 };

    let bricks = [];

    let score = 0;
    let isActive = false;
    let gameLoopId = null;

    // Colors
    const COLOR_PADDLE = 2; // Blue
    const COLOR_BALL = 1;   // White/Cyan
    const COLOR_BRICK = [7, 3, 4, 5]; // Red, Orange, Yellow, Green tiers
    const COLOR_BG = 0;

    function initGame() {
        paddleX = 6;
        ball = { x: 8, y: 13, dx: (Math.random() > 0.5 ? 1 : -1), dy: -1 };
        score = 0;
        isActive = true;

        // Create bricks (4 rows)
        bricks = [];
        for (let r = 0; r < 4; r++) {
            for (let c = 0; c < COLS; c += 2) { // Determine brick logic, pixel-wise? 
                // Let's make bricks 2px wide, 1px high
                bricks.push({ x: c, y: r + 1, color: COLOR_BRICK[r], active: true });
            }
        }

        updateScore();
        statusText.textContent = 'Pause';

        gameLoopId = requestAnimationFrame(gameLoop);
    }

    function gameLoop() {
        if (!isActive) return;

        update();
        sendFrame();

        setTimeout(() => {
            if (isActive) gameLoopId = requestAnimationFrame(gameLoop);
        }, 1000 / FPS);
    }

    function update() {
        // Move Ball
        ball.x += ball.dx;
        ball.y += ball.dy;

        // Wall collisions
        if (ball.x <= 0 || ball.x >= COLS - 1) ball.dx *= -1;
        if (ball.y <= 0) ball.dy *= -1; // Ceiling

        // Floor collision (Death)
        if (ball.y >= ROWS) {
            gameOver();
            return;
        }

        // Paddle collision
        // Paddle is at y = 15
        if (ball.y === 15) { // Check hitting top of paddle
            if (ball.x >= paddleX && ball.x < paddleX + paddleWidth) {
                ball.dy *= -1;
                // Add English?
                // ball.dx = ...
            }
        }

        // Brick collision
        for (let b of bricks) {
            if (b.active) {
                // Simple AABB for single points
                // Brick covers (x, y) and (x+1, y)
                if (ball.y === b.y && (ball.x === b.x || ball.x === b.x + 1)) {
                    b.active = false;
                    ball.dy *= -1;
                    score += 10;
                    updateScore();

                    // Check win?
                    if (bricks.every(br => !br.active)) {
                        gameOver(true);
                        return;
                    }
                    break;
                }
            }
        }
    }

    function packBoard() {
        const packed = new Uint8Array(128);
        const getPixel = (x, y) => {
            // Ball
            if (x === Math.floor(ball.x) && y === Math.floor(ball.y)) return COLOR_BALL;

            // Paddle
            if (y === 15 && x >= paddleX && x < paddleX + paddleWidth) return COLOR_PADDLE;

            // Bricks
            for (let b of bricks) {
                if (b.active && y === b.y && (x === b.x || x === b.x + 1)) return b.color;
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

        const packet = new Uint8Array(8 + 128);
        packet[0] = 5; // Header
        packet.set(packBoard(), 8);

        try { await char.writeValue(packet); } catch (e) { }
    }

    // Controls
    const movePaddle = (dir) => {
        paddleX += dir * 2;
        if (paddleX < 0) paddleX = 0;
        if (paddleX > COLS - paddleWidth) paddleX = COLS - paddleWidth;
    };

    // Repeat logic for paddle
    let repeatTimer = null;
    const startMoving = (dir) => {
        if (!isActive) return;
        movePaddle(dir);
        if (repeatTimer) clearInterval(repeatTimer);
        repeatTimer = setInterval(() => { if (isActive) movePaddle(dir); }, 100);
    };
    const stopMoving = () => { if (repeatTimer) clearInterval(repeatTimer); };

    const setupBtn = (btn, dir) => {
        btn.addEventListener('touchstart', (e) => { e.preventDefault(); startMoving(dir); }, { passive: false });
        btn.addEventListener('touchend', (e) => { e.preventDefault(); stopMoving(); });
        btn.addEventListener('mousedown', (e) => { e.preventDefault(); startMoving(dir); });
        btn.addEventListener('mouseup', (e) => { e.preventDefault(); stopMoving(); });
    };

    setupBtn(btnLeft, -1);
    setupBtn(btnRight, 1);

    function updateScore() {
        scoreText.textContent = score;
    }

    function gameOver(win = false) {
        isActive = false;
        statusText.textContent = win ? 'YOU WIN' : 'GAME OVER';
    }

    function stopGame() {
        isActive = false;
        statusText.textContent = 'Resume';
        cancelAnimationFrame(gameLoopId);
    }

    startBtn.addEventListener('click', () => {
        if (statusText.textContent === 'GAME OVER' || statusText.textContent === 'YOU WIN') {
            initGame();
        } else {
            isActive ? stopGame() : initGame();
        }
    });

    backBtn.addEventListener('click', () => {
        stopGame();
        window.showPage('modePage');
    });

    window.initArkanoidMode = function () {
        window.showPage('arkanoidPage');
    };

})();
