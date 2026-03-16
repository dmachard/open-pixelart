// ========== NIGHT LIGHT MODE ==========

window.nightLightConfig = {
    enabled: false,
    startH: 22,
    startM: 0,
    endH: 7,
    endM: 0,
    colorIndex: 0,
    brightness: 6
};

function initNightLightMode() {
    const cfg = window.nightLightConfig;

    // Helper to strip and re-attach event listeners
    const replaceElement = (id) => {
        const el = document.getElementById(id);
        if (!el) return null;
        const newEl = el.cloneNode(true);
        el.parentNode.replaceChild(newEl, el);
        return newEl;
    };

    // Populate UI from stored config
    const enabledEl = replaceElement('nightlightEnabled');
    const startEl = replaceElement('nightlightStart');
    const endEl = replaceElement('nightlightEnd');
    const saveBtn = replaceElement('nightlightSave');
    const backBtn = replaceElement('backFromNightlight');

    if (enabledEl) enabledEl.checked = cfg.enabled;
    if (startEl) startEl.value = `${String(cfg.startH).padStart(2, '0')}:${String(cfg.startM).padStart(2, '0')}`;
    if (endEl) endEl.value = `${String(cfg.endH).padStart(2, '0')}:${String(cfg.endM).padStart(2, '0')}`;

    // Back button
    if (backBtn) {
        backBtn.addEventListener('click', () => showPage('modePage'));
    }

    // Save button
    if (saveBtn) {
        saveBtn.addEventListener('click', async () => {
            // Read form values
            const enabled = document.getElementById('nightlightEnabled')?.checked ?? false;
            const startVal = document.getElementById('nightlightStart')?.value ?? '22:00';
            const endVal = document.getElementById('nightlightEnd')?.value ?? '07:00';

            // Hardcode color and brightness to 0 since we removed them from the UI
            const colorIdx = 0;
            const bright = 0;

            const [startH, startM] = startVal.split(':').map(Number);
            const [endH, endM] = endVal.split(':').map(Number);

            // Update global config
            window.nightLightConfig = { enabled, startH, startM, endH, endM, colorIndex: colorIdx, brightness: bright };

            // Send to device via BLE
            // Protocol: [0]=MODE_NIGHTLIGHT(7), [1]=brightness, [2]=colorIndex,
            //           [3]=startHour, [4]=startMinute, [5]=endHour, [6]=endMinute,
            //           [7]=enabled (0/1)
            try {
                const payload = new Uint8Array([
                    7,          // MODE_NIGHTLIGHT
                    bright,     // [1] brightness
                    colorIdx,   // [2] color index
                    startH,     // [3] start hour
                    startM,     // [4] start minute
                    endH,       // [5] end hour
                    endM,       // [6] end minute
                    enabled ? 1 : 0  // [7] enabled flag
                ]);

                const char = window.ledmatrix?.ble?.getCharacteristic?.();
                if (char) {
                    await char.writeValue(payload);
                    showNotification('✓ Veilleuse enregistrée');
                    console.log('NightLight config sent:', window.nightLightConfig);
                } else {
                    showNotification('⚠️ Non connecté', true);
                }
            } catch (err) {
                console.error('Error sending night-light config:', err);
                showNotification('✗ Erreur envoi veilleuse', true);
            }
        });
    }
}
