# How to Return to Factory Menu (Without Modifying Binaries)

## The Problem

When you load Meshtastic or LoRABLE from the factory menu, that application becomes the boot partition. From that point on, the ESP32 boots directly into that application every time you press reset. The factory menu never runs again.

Since you cannot modify the binaries of Meshtastic or LoRABLE to add a "return to factory" button check, you need a hardware-only method.

## The Solution: Rapid Reset Detection

The factory menu has been configured to **always stay as the factory boot partition**. Here's how it works:

### Normal Operation
1. You select an app in the factory menu and double-click to load it
2. Factory menu saves "current app" to NVS (Non-Volatile Storage)
3. Factory menu sets that app as the boot partition and reboots
4. That app loads and runs normally

### Returning to Factory Menu

**Method 1: Triple Reset (Recommended)**
1. Press the **RST button** 3 times in quick succession (within 10 seconds)
2. On the 3rd reset, the factory menu detects the rapid resets
3. Factory menu clears the "current app" and stays in menu mode
4. You can now select a different app or reconfigure

**Method 2: Hold PRG During Boot**
1. Press and hold the **PRG button**
2. While holding PRG, press **RST**
3. Keep holding PRG for 2-3 seconds after reset
4. Factory menu detects the held button and stays in menu mode

## Technical Details

The factory menu uses RTC memory (which persists across soft resets) to count consecutive boots:

- **Boot 1**: Normal - loads saved app if one exists
- **Boot 2** (within 10 sec): Increments counter, still loads app
- **Boot 3** (within 10 sec): Detects rapid resets, forces factory menu

The RTC memory is cleared when:
- You successfully enter the factory menu
- More than 10 seconds pass between resets
- You hold the PRG button during boot

## Limitations

- This requires the factory partition to remain as the boot partition
- Apps must be launched by setting them as boot partition and rebooting
- The rapid reset detection depends on RTC memory persisting
- Power loss (not just soft reset) will clear RTC memory

## Alternative Approaches (Require Binary Modification)

If you CAN modify the Meshtastic/LoRABLE binaries:

1. Add the PRG button check at the start of their `setup()` function
2. Use the code from `include/FactoryReturn.h`
3. See `RETURN_TO_FACTORY.md` for instructions

This allows holding PRG during boot to return to factory from within those apps.
