# Return to Factory Menu

## For Users

To return to the factory menu from any loaded application:

**Hold PRG Button During Boot:**
1. Press and hold the **PRG button** (GPIO 0)
2. Press the **RST button** (reset) while holding PRG
3. Keep holding PRG for 2-3 seconds after reset
4. Release PRG button
5. Device will reboot into factory menu

**Important:** This only works if your application includes the factory return code (see developer section below).

---

## For Developers

**CRITICAL:** Each application (Meshtastic, LoRABLE, etc.) must include the factory return check in its setup() function, otherwise users cannot return to the factory menu once your app is loaded.

### Quick Setup (Recommended)

### Manual Implementation (if you can't use the header)

Add this code at the **very beginning** of your setup() function:

```cpp
#include "esp_ota_ops.h"
#include "esp_partition.h"

#define PRG_BUTTON 0

void setup() {
  // MUST BE FIRST - Check for factory return before any other init
  pinMode(PRG_BUTTON, INPUT_PULLUP);
  delay(100);
  
  if (digitalRead(PRG_BUTTON) == LOW) {
    Serial.begin(115200);
    delay(100);
    Serial.println("PRG held - returning to factory...");
    
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
  Serial.begin(115200);
  // ... rest of your setup
}
```

---

### Example: Adding to LoRABLE Application

In your LoRABLE main.cpp, modify the setup() function:

```cpp
#include <Arduino.h>
#include "FactoryReturn.h"  // Add this include
// ... other includes

void setup() {
    // THIS MUST BE FIRST LINE IN SETUP
    checkFactoryReturn();
    
    // Now continue with existing setup code
    Serial.begin(115200);
    
    unsigned long serialStart = millis();
    while (!Serial && (millis() - serialStart < 3000)) {
        delay(100);
    }
    delay(500);
    
    Serial.println("\n\n====================================");
    Serial.println("        LoRABLE Starting...        ");
    // ... rest of existing setup
}
```

---

### Optional: Long Press Method

You can also add a long-press handler in your loop() for returning to factory:
  delay(50);
  
  // Check if button held during boot
  if (digitalRead(PRG_BUTTON) == LOW) {
    Serial.println("PRG held - returning to factory...");
    
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
```

### Option 2: Long press handler (in loop())

```cpp
#include "esp_ota_ops.h"
#include "esp_partition.h"

#define PRG_BUTTON 0
#define LONG_PRESS_TIME 3000

unsigned long buttonPressStart = 0;
bool buttonWasPressed = false;

void loop() {
  bool isPressed = (digitalRead(PRG_BUTTON) == LOW);
  
  // Button pressed
  if (isPressed && !buttonWasPressed) {
    buttonPressStart = millis();
    buttonWasPressed = true;
  }
  
  // Check for long press
  if (isPressed && buttonWasPressed) {
    if (millis() - buttonPressStart > LONG_PRESS_TIME) {
      Serial.println("Long press - returning to factory...");
      
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
      
      buttonWasPressed = false; // Prevent repeated triggers
    }
  }
  
  // Button released
  if (!isPressed && buttonWasPressed) {
    buttonWasPressed = false;
  }
  
  // Continue with normal loop...
}
```

### Full Example for LoRABLE

The LoRABLE application already has a long press callback. Add this function:

```cpp
void returnToFactory() {
  Serial.println("\n=== Returning to Factory Menu ===");
  
  // Show message on display
  display.clear();
  display.setFont(ArialMT_Plain_10);
  int width = display.getStringWidth("Returning to");
  int x = (128 - width) / 2;
  display.drawString(x, 20, "Returning to");
  width = display.getStringWidth("Factory Menu");
  x = (128 - width) / 2;
  display.drawString(x, 35, "Factory Menu");
  display.display();
  
  // Find factory partition
  const esp_partition_t* factory = esp_partition_find_first(
    ESP_PARTITION_TYPE_APP,
    ESP_PARTITION_SUBTYPE_ANY,
    "factory"
  );
  
  if (factory != NULL) {
    esp_ota_set_boot_partition(factory);
    Serial.println("Factory partition set. Rebooting...");
    delay(2000);
    esp_restart();
  } else {
    Serial.println("ERROR: Factory partition not found!");
    display.clear();
    display.drawString(10, 25, "Factory not found!");
    display.display();
    delay(2000);
  }
}
```

Then update the long press callback:

```cpp
void onLongPress() {
  returnToFactory();
}
```

---

## Technical Details

The factory menu application checks for a held PRG button during boot and will:
1. Detect if PRG button is LOW (pressed)
2. Check if current partition is NOT factory
3. Set factory partition as boot partition
4. Reboot immediately

This ensures you can always return to the factory menu regardless of which application is currently loaded.
