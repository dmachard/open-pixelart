.PHONY: help build flash clean monitor

help:
	@echo "Available commands:"
	@echo "  make build   - Compile the ESP32 firmware"
	@echo "  make flash   - Compile and flash the firmware to the device"
	@echo "  make clean   - Clean the build artifacts"
	@echo "  make monitor - Open the serial monitor"

build:
	cd firmware && pio run

flash:
	cd firmware && pio run -t upload

clean:
	cd firmware && pio run -t clean

monitor:
	cd firmware && pio device monitor
