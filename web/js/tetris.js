(function () {
    // Canvas removed, running headless

    const statusText = document.getElementById('tetrisStatusText');
    const scoreText = document.getElementById('tetrisScore');
    const startBtn = document.getElementById('startTetrisBtn');
    const backBtn = document.getElementById('backFromGame');

    const ROWS = 16;
    const COLS = 16; // 16x16 grid, but game well is 10 wide centered
    const WELL_WIDTH = 10;
    const WELL_OFFSET_X = 3;

    let board = [];
    let score = 0;
    let isActive = false;
    let gameLoopId = null;
    let currentPiece = null;
    let lastDropTime = 0;
    let dropInterval = 1000;

    // Tetromino definitions
    // Colors map to firmware palette: 
    // 1:Cyan(I), 2:Blue(J), 3:Orange(L), 4:Yellow(O), 5:Green(S), 6:Purple(T), 7:Red(Z)
    const SHAPES = [
        [], // 0 unused
        [[1, 1, 1, 1]], // I (Cyan)
        [[1, 0, 0], [1, 1, 1]], // J (Blue)
        [[0, 0, 1], [1, 1, 1]], // L (Orange)
        [[1, 1], [1, 1]], // O (Yellow)
        [[0, 1, 1], [1, 1, 0]], // S (Green)
        [[0, 1, 0], [1, 1, 1]], // T (Purple)
        [[1, 1, 0], [0, 1, 1]]  // Z (Red)
    ];

    const COLORS = [0, 1, 2, 3, 4, 5, 6, 7];

    function initBoard() {
        board = Array.from({ length: ROWS }, () => Array(COLS).fill(0));
        // Fill sides with grey (8) to visualize the well
        for (let r = 0; r < ROWS; r++) {
            for (let c = 0; c < COLS; c++) {
                if (c < WELL_OFFSET_X || c >= WELL_OFFSET_X + WELL_WIDTH) {
                    board[r][c] = 8; // Grey wall
                }
            }
        }
    }

    class Piece {
        constructor(shapeId) {
            this.shapeId = shapeId;
            this.color = COLORS[shapeId];
            this.shape = SHAPES[shapeId];
            // Center in well
            this.x = WELL_OFFSET_X + Math.floor((WELL_WIDTH - this.shape[0].length) / 2);
            this.y = 0; // Spawn at top
        }
    }

    function spawnPiece() {
        const shapeId = Math.floor(Math.random() * 7) + 1;
        currentPiece = new Piece(shapeId);
        // Check collision on spawn (Game Over)
        if (checkCollision(0, 0, currentPiece.shape)) {
            gameOver();
        }
    }

    function checkCollision(dx, dy, shape) {
        for (let r = 0; r < shape.length; r++) {
            for (let c = 0; c < shape[r].length; c++) {
                if (shape[r][c]) {
                    const newX = currentPiece.x + c + dx;
                    const newY = currentPiece.y + r + dy;
                    if (newX < 0 || newX >= COLS || newY >= ROWS || board[newY][newX]) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    function rotate(matrix) {
        const N = matrix.length;
        const M = matrix[0].length;
        const result = Array.from({ length: M }, () => Array(N).fill(0));
        for (let r = 0; r < N; r++) {
            for (let c = 0; c < M; c++) {
                result[c][N - 1 - r] = matrix[r][c];
            }
        }
        return result;
    }

    function tryRotate() {
        const nextShape = rotate(currentPiece.shape);
        // Basic wall kick (try sticking to center)
        if (!checkCollision(0, 0, nextShape)) {
            currentPiece.shape = nextShape;
        } else if (!checkCollision(-1, 0, nextShape)) {
            currentPiece.x -= 1;
            currentPiece.shape = nextShape;
        } else if (!checkCollision(1, 0, nextShape)) {
            currentPiece.x += 1;
            currentPiece.shape = nextShape;
        }
    }

    function lockPiece() {
        for (let r = 0; r < currentPiece.shape.length; r++) {
            for (let c = 0; c < currentPiece.shape[r].length; c++) {
                if (currentPiece.shape[r][c]) {
                    board[currentPiece.y + r][currentPiece.x + c] = currentPiece.color;
                }
            }
        }
        clearLines();
        spawnPiece();
    }

    function clearLines() {
        let linesCleared = 0;
        for (let r = ROWS - 1; r >= 0; r--) {
            let isFull = true;
            for (let c = WELL_OFFSET_X; c < WELL_OFFSET_X + WELL_WIDTH; c++) {
                if (!board[r][c]) {
                    isFull = false;
                    break;
                }
            }
            if (isFull) {
                // Remove line
                board.splice(r, 1);
                // Add new empty line at top, preserving walls
                const newLine = Array(COLS).fill(0);
                for (let i = 0; i < COLS; i++) {
                    if (i < WELL_OFFSET_X || i >= WELL_OFFSET_X + WELL_WIDTH) newLine[i] = 8;
                }
                board.unshift(newLine);
                linesCleared++;
                r++; // Check same row index again
            }
        }
        if (linesCleared) {
            score += linesCleared * 100;
            updateScore();
            // speed up
            dropInterval = Math.max(100, 1000 - (score / 100) * 50);
        }
    }

    function updateScore() {
        scoreText.textContent = score;
    }

    function gameOver() {
        isActive = false;
        statusText.textContent = 'GAME OVER';
        // alert('Game Over! Score: ' + score);
        window?.showGameOver?.();
    }



    function packBoard() {
        // Create 128 bytes buffer (16x16 pixels, 4 bits each)
        const packed = new Uint8Array(128);

        // Helper to get pixel color (from board or currentPiece)
        const getPixel = (x, y) => {
            // Check moving piece
            if (currentPiece &&
                x >= currentPiece.x && x < currentPiece.x + currentPiece.shape[0].length &&
                y >= currentPiece.y && y < currentPiece.y + currentPiece.shape.length) {

                const pr = y - currentPiece.y;
                const pc = x - currentPiece.x;
                if (currentPiece.shape[pr] && currentPiece.shape[pr][pc]) {
                    return currentPiece.color;
                }
            }
            return board[y][x];
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

        // Header: [mode=5, ...padding...]
        const header = new Uint8Array(8);
        header[0] = 5; // MODE_GAME

        const packet = new Uint8Array(8 + 128);
        packet.set(header, 0);
        packet.set(packedData, 8);

        try {
            await char.writeValue(packet);
        } catch (e) {
            // ignore
        }
    }

    function gameLoop(time) {
        if (!isActive) return;

        const dt = time - lastDropTime;
        if (dt > dropInterval) {
            if (!checkCollision(0, 1, currentPiece.shape)) {
                currentPiece.y++;
            } else {
                lockPiece();
            }
            lastDropTime = time;
        }


        sendFrame(); // Send every frame (or could limit FPS)

        gameLoopId = requestAnimationFrame(gameLoop);
    }

    let swipeHandler = null; // Swipe Handler

    function startGame() {
        window?.stopGameOver?.();
        if (isActive) return;

        // Init Swipe
        const swipeArea = document.getElementById('tetrisSwipeArea');
        if (swipeHandler) swipeHandler.detach();
        swipeHandler = new SwipeHandler(swipeArea, (dir) => {
            if (!isActive) return;
            if (dir === 'LEFT') moveLeft();
            if (dir === 'RIGHT') moveRight();
            if (dir === 'DOWN') moveDown();
            if (dir === 'UP') rotatePiece();
        });

        initBoard();
        score = 0;
        updateScore();
        spawnPiece();
        isActive = true;
        statusText.textContent = 'Pause';

        lastDropTime = performance.now();
        gameLoopId = requestAnimationFrame(gameLoop);
    }

    function stopGame() {
        window?.stopGameOver?.();
        if (swipeHandler) {
            swipeHandler.detach();
            swipeHandler = null;
        }
        isActive = false;
        statusText.textContent = 'Resume';
        cancelAnimationFrame(gameLoopId);
    }

    // Input handlers
    const moveLeft = () => { if (isActive && !checkCollision(-1, 0, currentPiece.shape)) currentPiece.x--; };
    const moveRight = () => { if (isActive && !checkCollision(1, 0, currentPiece.shape)) currentPiece.x++; };
    const moveDown = () => { if (isActive && !checkCollision(0, 1, currentPiece.shape)) currentPiece.y++; };
    const rotatePiece = () => { if (isActive) tryRotate(); };

    // Input repeat helpers
    let repeatTimer = null;

    const startRepeat = (action, delay = 50) => {
        if (!isActive) return;
        action();
        stopRepeat(); // clear any existing
        repeatTimer = setInterval(() => {
            if (isActive) action();
        }, delay);
    };

    const stopRepeat = () => {
        if (repeatTimer) {
            clearInterval(repeatTimer);
            repeatTimer = null;
        }
    };

    const setupBtn = (id, action, repeat = false) => {
        const btn = document.getElementById(id);
        const start = (e) => {
            e.preventDefault(); // prevent mouse emulation
            if (repeat) startRepeat(action);
            else action();
        };
        const end = (e) => {
            e.preventDefault();
            if (repeat) stopRepeat();
        };

        btn.addEventListener('touchstart', start, { passive: false });
        btn.addEventListener('touchend', end);
        btn.addEventListener('mousedown', start);
        btn.addEventListener('mouseup', end);
        btn.addEventListener('mouseleave', end);
    };

    setupBtn('btnLeft', moveLeft);
    setupBtn('btnRight', moveRight);
    setupBtn('btnDown', moveDown, true); // True for repeat
    setupBtn('btnRotate', rotatePiece);

    startBtn.addEventListener('click', () => isActive ? stopGame() : startGame());

    // Keyboard support
    document.addEventListener('keydown', (e) => {
        if (!isActive) return;
        switch (e.key) {
            case 'ArrowLeft': moveLeft(); break;
            case 'ArrowRight': moveRight(); break;
            case 'ArrowDown': moveDown(); break;
            case 'ArrowUp': rotatePiece(); break;
        }
    });

    backBtn.addEventListener('click', () => {
        stopGame();
        window.showPage('modePage');
    });

    window.initTetrisMode = function () {
        window.showPage('gamePage');
    };

})();
