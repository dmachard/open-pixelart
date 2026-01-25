// Swipe Gesture Handler
class SwipeHandler {
    constructor(element, onSwipe) {
        this.element = element;
        this.onSwipe = onSwipe; // callback(direction)
        this.touchStartX = 0;
        this.touchStartY = 0;
        this.minSwipeDistance = 20; // Lower threshold often feels faster
        this.hasSwiped = false; // For tap detection
        this.lastTapTime = 0;

        // For continuous swipe
        this.lastTriggerX = 0;
        this.lastTriggerY = 0;

        this.handleTouchStart = this.handleTouchStart.bind(this);
        this.handleTouchMove = this.handleTouchMove.bind(this);
        this.handleTouchEnd = this.handleTouchEnd.bind(this);

        this.attach();
    }

    attach() {
        this.element.addEventListener('touchstart', this.handleTouchStart, { passive: false });
        this.element.addEventListener('touchmove', this.handleTouchMove, { passive: false });
        this.element.addEventListener('touchend', this.handleTouchEnd, { passive: false });
    }

    detach() {
        this.element.removeEventListener('touchstart', this.handleTouchStart);
        this.element.removeEventListener('touchmove', this.handleTouchMove);
        this.element.removeEventListener('touchend', this.handleTouchEnd);
    }

    handleTouchStart(e) {
        this.touchStartX = e.changedTouches[0].screenX;
        this.touchStartY = e.changedTouches[0].screenY;

        this.lastTriggerX = this.touchStartX;
        this.lastTriggerY = this.touchStartY;

        this.hasSwiped = false; // Reset flag
    }

    handleTouchMove(e) {
        // Prevent scrolling while swiping on the game area
        if (e.cancelable) e.preventDefault();

        const touchCurrentX = e.changedTouches[0].screenX;
        const touchCurrentY = e.changedTouches[0].screenY;

        // Calculate distance from LAST TRIGGER (not start)
        const deltaX = touchCurrentX - this.lastTriggerX;
        const deltaY = touchCurrentY - this.lastTriggerY;

        // Check horizontal
        if (Math.abs(deltaX) > this.minSwipeDistance) {
            if (Math.abs(deltaX) > Math.abs(deltaY)) { // Ensure it's mostly horizontal
                if (deltaX > 0) this.onSwipe('RIGHT');
                else this.onSwipe('LEFT');

                // Reset origin for next step (Continuous Swipe)
                this.lastTriggerX = touchCurrentX;
                this.lastTriggerY = touchCurrentY;
                this.hasSwiped = true; // Mark as swiped effectively (for tap detection)
                return;
            }
        }

        // Check vertical
        if (Math.abs(deltaY) > this.minSwipeDistance) {
            if (Math.abs(deltaY) > Math.abs(deltaX)) { // Ensure it's mostly vertical
                if (deltaY > 0) this.onSwipe('DOWN');
                else this.onSwipe('UP');

                // Reset origin for next step
                this.lastTriggerX = touchCurrentX;
                this.lastTriggerY = touchCurrentY;
                this.hasSwiped = true;
                return;
            }
        }
    }

    handleTouchEnd(e) {
        if (!this.hasSwiped) {
            // It was a tap (no significant movement)
            const currentTime = new Date().getTime();
            const tapLength = currentTime - this.lastTapTime;

            if (tapLength < 300 && tapLength > 0) {
                // Double Tap detection
                this.onSwipe('DOUBLE_TAP');
                this.lastTapTime = 0; // Reset
            } else {
                this.lastTapTime = currentTime;
            }
        }
        this.hasSwiped = false;
    }
}

window.SwipeHandler = SwipeHandler;
