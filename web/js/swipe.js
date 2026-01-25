// Swipe Gesture Handler
class SwipeHandler {
    constructor(element, onSwipe) {
        this.element = element;
        this.onSwipe = onSwipe; // callback(direction)
        this.touchStartX = 0;
        this.touchStartY = 0;
        this.minSwipeDistance = 30; // px

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
    }

    handleTouchMove(e) {
        // Prevent scrolling while swiping on the game area
        if (e.cancelable) e.preventDefault();
    }

    handleTouchEnd(e) {
        const touchEndX = e.changedTouches[0].screenX;
        const touchEndY = e.changedTouches[0].screenY;

        const deltaX = touchEndX - this.touchStartX;
        const deltaY = touchEndY - this.touchStartY;

        if (Math.abs(deltaX) > Math.abs(deltaY)) {
            // Horizontal Swipe
            if (Math.abs(deltaX) > this.minSwipeDistance) {
                if (deltaX > 0) this.onSwipe('RIGHT');
                else this.onSwipe('LEFT');
            }
        } else {
            // Vertical Swipe
            if (Math.abs(deltaY) > this.minSwipeDistance) {
                if (deltaY > 0) this.onSwipe('DOWN');
                else this.onSwipe('UP');
            }
        }
    }
}

window.SwipeHandler = SwipeHandler;
