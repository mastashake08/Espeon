/**
 * INSTRUCTIONS FOR ADDING FACTORY RETURN TO YOUR APPLICATIONS
 * 
 * This is what you need to add to Meshtastic and LoRABLE to enable
 * returning to the factory menu by holding PRG during boot.
 */

// ============================================
// FOR LORABLE APPLICATION
// ============================================

// 1. Copy include/FactoryReturn.h to your LoRABLE project's include/ folder

// 2. Add this include near the top of your main.cpp:
#include "FactoryReturn.h"

// 3. Modify your setup() function to call checkFactoryReturn() FIRST:

void setup() {
    // THIS MUST BE THE FIRST LINE
    checkFactoryReturn();  // Checks for held PRG button and returns to factory if needed
    
    // Now continue with your existing setup code...
    Serial.begin(115200);
    
    unsigned long serialStart = millis();
    while (!Serial && (millis() - serialStart < 3000)) {
        delay(100);
    }
    delay(500);
    
    Serial.println("\n\n====================================");
    Serial.println("        LoRABLE Starting...        ");
    Serial.println("====================================");
    // ... rest of your existing setup code
}


// ============================================
// FOR MESHTASTIC APPLICATION
// ============================================

// 1. Copy include/FactoryReturn.h to your Meshtastic project's include/ folder

// 2. If Meshtastic is Arduino-based, add to main setup():

#include "FactoryReturn.h"

void setup() {
    // THIS MUST BE THE FIRST LINE
    checkFactoryReturn();
    
    // Continue with Meshtastic initialization...
}

// 3. If Meshtastic uses a different entry point, add checkFactoryReturn()
//    as the first line in the main initialization function


// ============================================
// ALTERNATIVE: MANUAL CODE (NO HEADER FILE)
// ============================================

// If you can't use the header file, add this at the start of setup():

void setup() {
    // Check for factory return
    pinMode(0, INPUT_PULLUP);  // PRG button is GPIO 0
    delay(100);
    
    if (digitalRead(0) == LOW) {
        // Button is held - return to factory
        const esp_partition_t* factory = esp_partition_find_first(
            ESP_PARTITION_TYPE_APP,
            ESP_PARTITION_SUBTYPE_ANY,
            "factory"
        );
        
        if (factory != NULL) {
            esp_ota_set_boot_partition(factory);
            delay(500);
            esp_restart();
        }
    }
    
    // Continue with normal setup...
}


// ============================================
// TESTING
// ============================================

// After adding the code:
// 1. Upload your modified application
// 2. Load it from the factory menu
// 3. Once it's running, hold PRG and press RST
// 4. Keep holding PRG for 2 seconds
// 5. Release - you should boot back to factory menu

// Serial output should show:
//   "!!! PRG BUTTON HELD DURING BOOT !!!"
//   "Returning to factory menu..."
//   "Current partition: LoRABLE" (or whatever app you're in)
//   "Switching to: factory"
//   "SUCCESS: Factory partition set"
//   "Rebooting to factory menu..."
