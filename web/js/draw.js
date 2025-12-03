
// ========== DRAW MODE ==========

let gridWidth = 16;
let gridHeight = 16;
let pixels = Array(16).fill().map(() => Array(16).fill(null));
let pixelElements = [];
let brightness = 25;
let selectedColor = '#ffffff';
let isDrawing = false;
let lastDrawState = true;
let lastPixel = null;
let sendTimeout = null;

function initDrawMode() {
    // Use device dimensions if available, otherwise default to 16x16
    gridWidth = currentDeviceInfo?.width || 16;
    gridHeight = currentDeviceInfo?.height || 16;
    
    // Reinitialize pixels array if dimensions changed
    pixels = Array(gridHeight).fill().map(() => Array(gridWidth).fill(null));
    
    if (pixelElements.length === 0) {
        createGrid();
        initPalette();
        initDrawControls();
    }
}

function createGrid() {
    const gridDiv = document.getElementById('grid');
    gridDiv.innerHTML = '';
    pixelElements = [];
    
    for (let y = 0; y < gridHeight; y++) {
        pixelElements[y] = [];
        for (let x = 0; x < gridWidth; x++) {
            const pixel = document.createElement('div');
            pixel.className = 'pixel';
            pixel.dataset.x = x;
            pixel.dataset.y = y;
            gridDiv.appendChild(pixel);
            pixelElements[y][x] = pixel;
        }
    }
    
    gridDiv.addEventListener('pointerdown', startDrawing);
    gridDiv.addEventListener('pointermove', continueDrawing);
    gridDiv.addEventListener('pointerup', () => { isDrawing = false; });
    gridDiv.addEventListener('pointerleave', () => { isDrawing = false; });
}

function initPalette() {
    const swatches = document.querySelectorAll('.swatch');
    const eraser = document.getElementById('eraser');
    const customColor = document.getElementById('customColor');
    
    function selectSwatch(el) {
        swatches.forEach(s => s.classList.remove('selected'));
        if (el && el.classList.contains('swatch')) el.classList.add('selected');
    }
    
    const initial = Array.from(swatches).find(s => s.dataset.color === selectedColor);
    if (initial) selectSwatch(initial);
    
    swatches.forEach(s => {
        s.addEventListener('click', () => {
            selectedColor = s.dataset.color || null;
            selectSwatch(s);
        });
    });
    
    eraser.addEventListener('click', () => {
        selectedColor = null;
        selectSwatch(null);
    });
    
    customColor.addEventListener('input', (e) => {
        selectedColor = e.target.value;
        selectSwatch(null);
    });
}

function initDrawControls() {
    document.getElementById('backFromDraw').addEventListener('click', () => showPage('modePage'));
    
    document.getElementById('clearBtn').addEventListener('click', () => {
        for (let y = 0; y < 16; y++) {
            for (let x = 0; x < 16; x++) {
                setPixelColor(x, y, null);
            }
        }
    });
    
    document.getElementById('saveBtn').addEventListener('click', () => {
        const drawing = {
            version: '1.0',
            width: 16,
            height: 16,
            brightness: brightness,
            pixels: pixels,
            timestamp: new Date().toISOString()
        };
        const json = JSON.stringify(drawing, null, 2);
        const blob = new Blob([json], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `led-matrix-${Date.now()}.json`;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
        showNotification('✓ Drawing saved!');
    });
    
    document.getElementById('loadBtn').addEventListener('click', () => {
        document.getElementById('fileInput').click();
    });
    
    document.getElementById('fileInput').addEventListener('change', (e) => {
        const file = e.target.files[0];
        if (!file) return;
        const reader = new FileReader();
        reader.onload = (event) => {
            try {
                const drawing = JSON.parse(event.target.result);
                if (!drawing.pixels || !Array.isArray(drawing.pixels)) {
                    throw new Error('Invalid file format');
                }
                if (drawing.brightness !== undefined) {
                    brightness = drawing.brightness;
                    document.getElementById('brightnessSelect').value = brightness;
                }
                for (let y = 0; y < 16; y++) {
                    for (let x = 0; x < 16; x++) {
                        const color = drawing.pixels[y][x] || null;
                        setPixelColor(x, y, color);
                    }
                }

                // sendToESP32Debounced();

                showNotification('✓ Drawing loaded!');
            } catch (err) {
                showNotification('✗ Error loading file', true);
            }
        };
        reader.readAsText(file);
    });
    
    // document.getElementById('sendDrawBtn').addEventListener('click', sendCurrentDrawing);
    
    document.getElementById('brightnessSelect').addEventListener('change', (e) => {
        brightness = parseInt(e.target.value);
    });
}

function startDrawing(e) {
    e.preventDefault();
    isDrawing = true;
    lastPixel = null;
    
    const pixel = e.target.closest('.pixel');
    if (!pixel) return;
    
    const x = parseInt(pixel.dataset.x);
    const y = parseInt(pixel.dataset.y);
    
    lastDrawState = !(pixels[y][x] !== null);
    lastPixel = pixel;
    setPixelColor(x, y, lastDrawState ? selectedColor : null);
}

function continueDrawing(e) {
    if (!isDrawing) return;
    e.preventDefault();
    
    const element = document.elementFromPoint(e.clientX, e.clientY);
    const pixel = element?.closest('.pixel');
    if (!pixel || pixel === lastPixel) return;
    
    const x = parseInt(pixel.dataset.x);
    const y = parseInt(pixel.dataset.y);
    
    lastPixel = pixel;
    setPixelColor(x, y, lastDrawState ? selectedColor : null);
}

function setPixelColor(x, y, color) {
    const normalizedColor = color === '#000000' ? null : color;
    pixels[y][x] = normalizedColor;
    
    const el = pixelElements[y][x];
    if (normalizedColor) {
        el.classList.add('on');
        el.style.background = normalizedColor;
        el.style.boxShadow = '0 0 8px ' + normalizedColor;
    } else {
        el.classList.remove('on');
        el.style.background = '#000';
        el.style.boxShadow = 'none';
    }

    sendToESP32Debounced();
}

function preparePixelsAndPalette(pixelsGrid = pixels) {
    const usedColors = new Set();
    for (let y = 0; y < 16; y++) {
        for (let x = 0; x < 16; x++) {
            if (pixelsGrid[y][x]) usedColors.add(pixelsGrid[y][x]);
        }
    }
    
    const palette = ['#000000'];
    usedColors.forEach(c => {
        if (c !== '#000000') palette.push(c);
    });
    const finalPalette = palette.slice(0, 16);
    
    const colorToIndex = {};
    finalPalette.forEach((color, idx) => (colorToIndex[color] = idx));
    
    const pixelsFlat = [];
    for (let y = 0; y < 16; y++) {
        for (let x = 0; x < 16; x++) {
            pixelsFlat.push(colorToIndex[pixelsGrid[y][x] || '#000000'] || 0);
        }
    }
    
    return { pixelsFlat, palette: finalPalette };
}

async function sendCurrentDrawing() {
    try {
        const { pixelsFlat, palette } = preparePixelsAndPalette();
        await window.ledmatrix.esp32.send({
            pixels: pixelsFlat,
            palette: palette,
            brightness: brightness,
            mode: 0,
            transition: 1
        });
        showNotification('✓ Drawing sent!');
    } catch (err) {
        showNotification('✗ Error sending drawing', true);
    }
}


function sendToESP32Debounced() {
    if (!window.ledmatrix || !window.ledmatrix.esp32 || !window.ledmatrix.esp32.send) {
        console.warn('LEDMatrix not connected, skipping send');
        return;
    }
    if (sendTimeout) clearTimeout(sendTimeout);
    
    sendTimeout = setTimeout(() => {
        // Convert pixels and palette
        const { pixelsFlat, palette } = preparePixelsAndPalette();

        // Send to ESP32
        window.ledmatrix.esp32.send({  
            pixels: pixelsFlat, 
            palette: palette,
            brightness: brightness, 
            mode: 0,
            transition: 1 });
        sendTimeout = null;
    }, 20);

}

function preparePixelsAndPalette(pixelsGrid = pixels) {
    const usedColors = new Set();
    for (let y = 0; y < 16; y++)
        for (let x = 0; x < 16; x++)
            if (pixelsGrid[y][x]) usedColors.add(pixelsGrid[y][x]);

    const palette = ['#000000'];
    usedColors.forEach(c => {
        if (c !== '#000000') palette.push(c);
    });
    const finalPalette = palette.slice(0, 16);

    const colorToIndex = {};
    finalPalette.forEach((color, idx) => (colorToIndex[color] = idx));

    const pixelsFlat = [];
    for (let y = 0; y < 16; y++)
        for (let x = 0; x < 16; x++)
            pixelsFlat.push(colorToIndex[pixelsGrid[y][x] || '#000000'] || 0);

    return { pixelsFlat, palette: finalPalette };
}
