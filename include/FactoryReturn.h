/**
 * FactoryReturn.h
 * 
 * Include this in any application that uses the Espeon multi-partition system
 * to enable returning to the factory menu by holding PRG during boot.
 * 
 * Usage in your app's setup():
 * 
 *   #include "FactoryReturn.h"
 *   
 *   void setup() {
 *     checkFactoryReturn();  // Call this FIRST, before any other initialization
 *     
 *     // ... rest of your setup code
 *   }
 */

#ifndef FACTORY_RETURN_H
#define FACTORY_RETURN_H

#include <Arduino.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"

#define PRG_BUTTON 0
#define FACTORY_PARTITION_NAME "factory"

/**
 * Check if PRG button is held during boot and return to factory partition if so.
 * Call this FIRST in your setup() function, before any other initialization.
 */
void checkFactoryReturn() {
  // Initialize PRG button
  pinMode(PRG_BUTTON, INPUT_PULLUP);
  delay(100);  // Give pin time to stabilize
  
  // Check if button is held (LOW = pressed)
  bool buttonHeld = (digitalRead(PRG_BUTTON) == LOW);
  
  if (buttonHeld) {
    // Initialize serial for debugging (optional, comment out if not needed)
    Serial.begin(115200);
    delay(100);
    Serial.println("\n!!! PRG BUTTON HELD DURING BOOT !!!");
    Serial.println("Returning to factory menu...");
    
    // Get current and factory partitions
    const esp_partition_t* current = esp_ota_get_running_partition();
    const esp_partition_t* factory = esp_partition_find_first(
      ESP_PARTITION_TYPE_APP,
      ESP_PARTITION_SUBTYPE_ANY,
      FACTORY_PARTITION_NAME
    );
    
    // Only switch if factory partition exists and we're not already there
    if (factory != NULL && current != factory) {
      Serial.print("Current partition: ");
      Serial.println(current->label);
      Serial.print("Switching to: ");
      Serial.println(factory->label);
      
      // Set factory partition as boot partition
      esp_err_t err = esp_ota_set_boot_partition(factory);
      
      if (err == ESP_OK) {
        Serial.println("SUCCESS: Factory partition set");
        Serial.println("Rebooting to factory menu...");
        Serial.flush();
        delay(500);
        esp_restart();
        // Never reaches here
      } else {
        Serial.printf("ERROR: Failed to set boot partition (error code: %d)\n", err);
        Serial.println("Continuing with current app...");
        delay(2000);
      }
    } else if (factory == NULL) {
      Serial.println("ERROR: Factory partition not found!");
      delay(2000);
    } else {
      Serial.println("Already in factory partition");
      delay(1000);
    }
  }
}

#endif // FACTORY_RETURN_H
