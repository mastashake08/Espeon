#!/bin/bash

# Upload all partitions script
# Usage: ./upload_all.sh /dev/cu.usbserial-XXXX path/to/Meshtastic.bin path/to/LoRABLE.bin

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

FACTORY_BIN=".pio/build/heltec_wifi_lora_32_V3/firmware.bin"

echo ""
echo "Uploading all partitions..."
echo "  Factory:    0x010000 -> $FACTORY_BIN"
echo "  Meshtastic: 0x210000 -> $MESHTASTIC_BIN"
echo "  LoRABLE:    0x410000 -> $LORABLE_BIN"
echo ""

esptool.py --chip esp32s3 --port $PORT --baud 921600 \
    --before=default_reset --after=hard_reset write_flash \
    0x0 ~/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32s3/bin/bootloader_qio_80m.bin \
    0x8000 .pio/build/heltec_wifi_lora_32_V3/partitions.bin \
    0xe000 ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
    0x10000 $FACTORY_BIN \
    0x210000 $MESHTASTIC_BIN \
    0x410000 $LORABLE_BIN

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ All partitions uploaded successfully!"
else
    echo ""
    echo "✗ Upload failed!"
    exit 1
fi
