#!/bin/bash

# Upload all partitions script
# Usage: ./upload_all.sh /dev/cu.usbserial-XXXX path/to/Meshtastic.bin path/to/LoRABLE.bin

# Activate virtual environment if it exists
if [ -f ".venv/bin/activate" ]; then
    echo "Activating virtual environment..."
    source .venv/bin/activate
    
    # Check Python version
    PYTHON_VERSION=$(python --version 2>&1 | awk '{print $2}' | cut -d. -f1,2)
    PYTHON_MINOR=$(echo $PYTHON_VERSION | cut -d. -f2)
    
    if [ "$PYTHON_MINOR" -gt 13 ]; then
        echo "ERROR: Python $PYTHON_VERSION is not supported by PlatformIO"
        echo "Please recreate .venv with Python 3.11-3.13:"
        echo "  rm -rf .venv"
        echo "  python3.11 -m venv .venv"
        echo "  source .venv/bin/activate"
        echo "  pip install platformio esptool"
        exit 1
    fi
    
    echo "Using Python $PYTHON_VERSION"
fi

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <serial_port> <meshtastic.bin> <lorable.bin>"
    echo "Example: $0 /dev/cu.usbserial-0001 Meshtastic.bin LoRABLE.bin"
    exit 1
fi

PORT=$1
MESHTASTIC_BIN=$2
LORABLE_BIN=$3

echo "Building factory menu..."
pio run

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo ""
echo "Step 1: Uploading factory firmware (with bootloader and partitions)..."
pio run --target upload --upload-port $PORT

if [ $? -ne 0 ]; then
    echo "Factory firmware upload failed!"
    exit 1
fi

echo ""
echo "Step 2: Uploading additional applications..."
echo "  Meshtastic: 0x210000 -> $MESHTASTIC_BIN"

esptool.py --chip esp32s3 --port $PORT --baud 921600 \
    --before=default_reset --after=no_reset write_flash \
    0x210000 $MESHTASTIC_BIN

if [ $? -ne 0 ]; then
    echo "Meshtastic upload failed!"
    exit 1
fi

echo "  LoRABLE:    0x410000 -> $LORABLE_BIN"

esptool.py --chip esp32s3 --port $PORT --baud 921600 \
    --before=no_reset --after=hard_reset write_flash \
    0x410000 $LORABLE_BIN

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ All partitions uploaded successfully!"
    echo "  - Factory menu at 0x010000"
    echo "  - Meshtastic at 0x210000"
    echo "  - LoRABLE at 0x410000"
    echo ""
    echo "The device will now boot into the factory menu."
else
    echo ""
    echo "✗ LoRABLE upload failed!"
    exit 1
fi
