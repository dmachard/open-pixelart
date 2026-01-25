(function () {
    const statusText = document.getElementById('snakeStatusText');
    const scoreText = document.getElementById('snakeScore');
    const startBtn = document.getElementById('startSnakeBtn');
    const backBtn = document.getElementById('backFromSnake');

    const ROWS = 16;
    const COLS = 16;

    // Game state
    let snake = [];
    let food = null;
    let direction = { x: 1, y: 0 }; // Moving right initially
    let nextDirection = { x: 1, y: 0 };
    let score = 0;
    let isActive = false;
    let gameLoopId = null;
    let lastMoveTime = 0;
    let moveInterval = 200; // ms

    // Colors
    const COLOR_SNAKE = 5; // Green
    const COLOR_FOOD = 7;  // Red
    const COLOR_BG = 0;

    function initGame() {
        snake = [
            { x: 3, y: 8 },
            { x: 2, y: 8 },
            { x: 1, y: 8 }
        ];
        direction = { x: 1, y: 0 };
        nextDirection = { x: 1, y: 0 };
        score = 0;
        moveInterval = 200;
        isActive = true;

        spawnFood();
        updateScore();
        statusText.textContent = 'Pause';

        lastMoveTime = performance.now();
        gameLoopId = requestAnimationFrame(gameLoop);
    }

    function spawnFood() {
        while (true) {
            const x = Math.floor(Math.random() * COLS);
            const y = Math.floor(Math.random() * ROWS);
            // Check if on snake
            if (!snake.some(s => s.x === x && s.y === y)) {
                food = { x, y };
                break;
            }
        }
    }

    function gameLoop(time) {
        if (!isActive) return;

        const dt = time - lastMoveTime;
        if (dt > moveInterval) {
            update();
            lastMoveTime = time;
        }

        sendFrame();
        gameLoopId = requestAnimationFrame(gameLoop);
    }

    function update() {
        // Apply queued direction
        direction = nextDirection;

        const head = { x: snake[0].x + direction.x, y: snake[0].y + direction.y };

        // Wall collision
        if (head.x < 0 || head.x >= COLS || head.y < 0 || head.y >= ROWS) {
            gameOver();
            return;
        }

        // Self collision
        if (snake.some(s => s.x === head.x && s.y === head.y)) {
            gameOver();
            return;
        }

        snake.unshift(head); // Add new head

        // Check food
        if (head.x === food.x && head.y === food.y) {
            score++;
            updateScore();
            spawnFood();
            // Speed up slightly
            moveInterval = Math.max(50, 200 - (score * 2));
        } else {
            snake.pop(); // Remove tail
        }
    }

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

    function packBoard() {
        const packed = new Uint8Array(128);

        const getPixel = (x, y) => {
            if (food && food.x === x && food.y === y) return COLOR_FOOD;
            if (snake.some(s => s.x === x && s.y === y)) return COLOR_SNAKE;
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

        // Header: [mode=5, ...padding...] (Reusing GAME mode for simplicity)
        // Or we could define a new mode if firmware requires it, but raw pixels work with 5
        const header = new Uint8Array(8);
        header[0] = 5;

        const packet = new Uint8Array(8 + 128);
        packet.set(header, 0);
        packet.set(packedData, 8);

        try {
            await char.writeValue(packet);
        } catch (e) {
            // ignore
        }
    }

    // Controls
    const setDir = (x, y) => {
        // Prevent 180 turn
        if (x !== 0 && direction.x !== 0) return;
        if (y !== 0 && direction.y !== 0) return;
        nextDirection = { x, y };
    };

    // Touch/Mouse handlers with repeat logic are NOT needed for Snake direction changes,
    // just single tap is enough.
    const setupBtn = (id, x, y) => {
        const btn = document.getElementById(id);
        const handle = (e) => {
            e.preventDefault();
            if (isActive) setDir(x, y);
        };
        btn.addEventListener('touchstart', handle, { passive: false });
        btn.addEventListener('mousedown', handle);
    };

    setupBtn('btnSnakeUp', 0, -1);
    setupBtn('btnSnakeDown', 0, 1);
    setupBtn('btnSnakeLeft', -1, 0);
    setupBtn('btnSnakeRight', 1, 0);

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

    window.initSnakeMode = function () {
        window.showPage('snakePage');
    };

})();
