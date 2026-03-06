#!/bin/bash

# Initialize OTA data partition for ESP32-S3
# This allows the bootloader to properly switch between OTA partitions

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <serial_port>"
    echo "Example: $0 /dev/cu.usbserial-0001"
    exit 1
fi

PORT=$1

echo "Downloading boot_app0.bin (OTA data initializer)..."
BOOT_APP0="~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"

# Expand tilde
BOOT_APP0="${BOOT_APP0/#\~/$HOME}"

if [ ! -f "$BOOT_APP0" ]; then
    echo "ERROR: boot_app0.bin not found at $BOOT_APP0"
    echo "Installing Arduino ESP32 framework..."
    ~/.platformio/penv/bin/pio pkg install -p espressif32
fi

echo "Writing boot_app0.bin to otadata partition (0xe000)..."
esptool.py --chip esp32s3 --port $PORT --baud 921600 \
    write_flash 0xe000 "$BOOT_APP0"

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ OTA data partition initialized!"
    echo "The bootloader can now properly switch between OTA partitions."
else
    echo "Failed to initialize OTA data!"
    exit 1
fi
