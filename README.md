# Espeon - Multi-Partition Menu System for Heltec LoRa 32 V3

A factory menu application for the Heltec WiFi LoRa 32 V3 that allows switching between multiple applications stored in different flash partitions.

## Hardware

- **Board**: Heltec WiFi LoRa 32 V3
- **MCU**: ESP32-S3FN8 (8MB Flash)
- **Display**: 0.96" OLED (128x64 SSD1306)
- **Radio**: LoRa SX1262
- **Features**: WiFi, Bluetooth, Battery Management

## Overview

This project implements a multi-partition boot system that allows you to store multiple independent applications on the ESP32 and switch between them using a menu interface. The factory partition contains the menu system, which always remains accessible.

### Partition Layout

| Partition   | Type    | Offset     | Size | Description                    |
|-------------|---------|------------|------|--------------------------------|
| nvs         | data    | 0x9000     | 20KB | Non-volatile storage           |
| otadata     | data    | 0xe000     | 8KB  | OTA boot partition tracker     |
| factory     | app     | 0x10000    | 2MB  | Factory menu (this project)    |
| Meshtastic  | app     | 0x210000   | 2MB  | Meshtastic mesh network app    |
| LoRABLE     | app     | 0x410000   | 2MB  | LoRa + BLE application         |
| spiffs      | data    | 0x610000   | ~2MB | File system storage            |

## Features

- **PRG Button Navigation**: Single-click to navigate through applications
- **Double-Click Loading**: Double-click to boot into selected application
- **OLED Display**: Visual menu with battery voltage display
- **Partition Validation**: Checks partition integrity before switching
- **Error Handling**: Visual feedback for errors and invalid partitions
- **Serial Debug**: Comprehensive logging for troubleshooting

## Usage

### Controls

- **Single Click PRG Button**: Navigate through available applications
- **Double Click PRG Button**: Load and boot the selected application
- **Menu Display**: Shows application name and navigation instructions

The factory menu will always be accessible - if an application needs to return to the menu, it can set the boot partition back to "factory" and reboot.

## Building & Uploading

### Prerequisites

- [PlatformIO](https://platformio.org/)
- USB connection to Heltec LoRa 32 V3

### Build Factory Menu

```bash
pio run
```

### Upload All Partitions

#### Option 1: Using the Upload Script

```bash
./upload_all.sh /dev/cu.usbserial-XXXX path/to/Meshtastic.bin path/to/LoRABLE.bin
```

This will:
1. Build the factory menu
2. Upload bootloader and partition table
3. Upload all three applications to their respective partitions

#### Option 2: Manual Upload

```bash
# 1. Upload factory menu
pio run --target upload

# 2. Upload Meshtastic binary
esptool.py --chip esp32s3 --port /dev/cu.usbserial-XXXX --baud 921600 write_flash 0x210000 Meshtastic.bin

# 3. Upload LoRABLE binary
esptool.py --chip esp32s3 --port /dev/cu.usbserial-XXXX --baud 921600 write_flash 0x410000 LoRABLE.bin
```

### Finding Your Serial Port

**macOS:**
```bash
ls /dev/cu.*
```
Look for `/dev/cu.usbserial-XXXX` or `/dev/cu.SLAB_USBtoUART`

**Linux:**
```bash
ls /dev/ttyUSB*
```

**Windows:**
Check Device Manager for COM ports (e.g., `COM3`)

## Development

### Project Structure

```
Espeon/
├── src/
│   └── main.cpp           # Factory menu application
├── include/
├── lib/
├── partitions.csv         # Partition table definition
├── platformio.ini         # PlatformIO configuration
├── upload_all.sh          # Upload script for all partitions
└── README.md
```

### Adding New Applications

To add a new application partition:

1. **Update `partitions.csv`**: Add a new partition entry
2. **Update `main.cpp`**: Add the partition to the `appPartitions[]` array
3. **Adjust offsets**: Ensure partition offsets don't overlap
4. **Build and upload**: Upload the new binary to the correct offset

Example partition entry:
```csv
myapp, app, ota_3, 0x610000, 0x200000,
```

### Dependencies

- [Heltec ESP32 LoRa V3 Library](https://github.com/ropg/heltec_esp32_lora_v3)

### Button Configuration

- **PRG Button**: GPIO 0 (with internal pullup)
- **Debounce Delay**: 50ms
- **Double-Click Window**: 300ms
- **Long Press Threshold**: 2000ms (reserved for future use)

## Serial Monitor

Connect at 115200 baud to see debug output:

```bash
pio device monitor
```

The serial output shows:
- Current running partition
- Available partitions with addresses
- Button events (clicks, partition loading)
- Error messages

## Troubleshooting

### Partition Not Found
- Verify the binary was uploaded to the correct address
- Check that the partition label matches the name in `partitions.csv`

### Invalid Partition
- The uploaded binary may be corrupted
- Try re-uploading the application binary

### Button Not Responding
- Ensure GPIO 0 is not being used by other hardware
- Check serial output for button press detection

### Display Issues
- Verify the Heltec library is properly installed
- Check that `heltec_setup()` is called in `setup()`

## License

MIT License

## Credits

Built for the Heltec WiFi LoRa 32 V3 using:
- ESP32 Arduino Framework
- Heltec ESP32 LoRa V3 Library by @ropg
