#!/bin/bash

# Reset to Factory Script
# This script resets the boot partition to factory using esptool
# Use this when you need to return to the factory menu from a loaded app

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <serial_port>"
    echo "Example: $0 /dev/cu.usbserial-0001"
    echo ""
    echo "This script resets the OTA data partition to boot to factory."
    exit 1
fi

PORT=$1

echo "======================================"
echo "  Reset to Factory Partition"
echo "======================================"
echo ""
echo "This will reset the device to boot into the factory menu."
echo "Port: $PORT"
echo ""
echo "Press Enter to continue or Ctrl+C to cancel..."
read

# Step 1: Clear NVS to remove saved default app
echo "Step 1: Clearing NVS (saved preferences)..."
esptool.py --chip esp32s3 --port $PORT --baud 921600 \
    --before=default_reset --after=no_reset \
    erase_region 0x9000 0x5000

if [ $? -ne 0 ]; then
    echo "✗ Failed to clear NVS"
    exit 1
fi

# Step 2: The boot_app0.bin file forces the bootloader to use the factory partition
echo ""
echo "Step 2: Writing boot partition data..."

esptool.py --chip esp32s3 --port $PORT --baud 921600 \
    --before=no_reset --after=hard_reset write_flash \
    0xe000 ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin

if [ $? -eq 0 ]; then
    echo  ""
    echo "✓ Success! Device will now boot to factory menu."
    echo "  - NVS cleared (no saved default app)"
    echo "  - OTA data reset to factory partition"
    echo ""
else
    echo ""
    echo "✗ Failed to reset to factory"
    echo ""
    echo "Alternative method:"
    echo "  1. Find boot_app0.bin in your PlatformIO installation"
    echo "  2. Flash it to address 0xe000"
    echo ""
    exit 1
fi
