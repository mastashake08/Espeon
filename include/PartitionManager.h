/**
 * PartitionManager.h
 * 
 * Manages partition switching with automatic rollback to factory.
 * Uses NVS to track boot attempts and automatically return to factory
 * if an app fails to boot or after a configurable timeout.
 */

#ifndef PARTITION_MANAGER_H
#define PARTITION_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"

#define BOOT_CHECK_NAMESPACE "espeon"
#define BOOT_COUNT_KEY "boot_count"
#define BOOT_TIME_KEY "boot_time"
#define TARGET_PARTITION_KEY "target_part"
#define MAX_BOOT_ATTEMPTS 3
#define AUTO_RETURN_TIMEOUT 120000  // 2 minutes in milliseconds

class PartitionManager {
private:
    Preferences prefs;
    
public:
    /**
     * Check if we should return to factory based on boot count or timeout.
     * Call this EARLY in factory menu setup().
     * Returns true if we intercepted and are returning to factory.
     */
    bool checkAndHandleReturn() {
        prefs.begin(BOOT_CHECK_NAMESPACE, false);
        
        uint32_t bootCount = prefs.getUInt(BOOT_COUNT_KEY, 0);
        unsigned long bootTime = prefs.getULong(BOOT_TIME_KEY, 0);
        String targetPartition = prefs.getString(TARGET_PARTITION_KEY, "");
        
        const esp_partition_t* current = esp_ota_get_running_partition();
        
        Serial.println("\n=== Partition Manager Check ===");
        Serial.printf("Current partition: %s\n", current->label);
        Serial.printf("Boot count: %d\n", bootCount);
        Serial.printf("Target partition: %s\n", targetPartition.c_str());
        
        bool shouldReturnToFactory = false;
        String reason = "";
        
        // Check if we're in the factory partition
        if (strcmp(current->label, "factory") == 0) {
            // We're in factory - check if we got here due to a failed boot
            if (bootCount > 0 && targetPartition.length() > 0) {
                reason = "App failed to boot after " + String(bootCount) + " attempts";
                shouldReturnToFactory = false;  // Already in factory
                
                // Clear the tracking
                prefs.clear();
                prefs.end();
                
                Serial.println("Already in factory after boot failure");
                Serial.println("Boot tracking cleared");
                
                return false;  // Don't need to do anything, already in factory
            }
            
            prefs.end();
            return false;
        }
        
        // We're in a loaded app - check conditions
        if (targetPartition.length() > 0 && strcmp(current->label, targetPartition.c_str()) == 0) {
            // We successfully booted into the target app
            
            // Check timeout
            unsigned long currentTime = millis();
            if (bootTime > 0 && (currentTime - bootTime) > AUTO_RETURN_TIMEOUT) {
                reason = "Auto-return timeout expired";
                shouldReturnToFactory = true;
            }
            // Check boot attempts
            else if (bootCount >= MAX_BOOT_ATTEMPTS) {
                reason = "Max boot attempts reached";
                shouldReturnToFactory = true;
            }
            // Increment boot count
            else {
                bootCount++;
                prefs.putUInt(BOOT_COUNT_KEY, bootCount);
                prefs.putULong(BOOT_TIME_KEY, millis());
                Serial.printf("Boot attempt %d recorded\n", bootCount);
            }
        }
        
        prefs.end();
        
        if (shouldReturnToFactory) {
            Serial.println("RETURNING TO FACTORY: " + reason);
            returnToFactory();
            return true;  // Never actually returns, device reboots
        }
        
        return false;
    }
    
    /**
     * Confirm that the current app is working properly.
     * Call this after your app successfully initializes.
     * Clears boot tracking to prevent automatic return to factory.
     */
    void confirmBoot() {
        prefs.begin(BOOT_CHECK_NAMESPACE, false);
        
        String targetPartition = prefs.getString(TARGET_PARTITION_KEY, "");
        const esp_partition_t* current = esp_ota_get_running_partition();
        
        if (targetPartition.length() > 0 && strcmp(current->label, targetPartition.c_str()) == 0) {
            Serial.println("\n=== Boot Confirmed ===");
            Serial.printf("App '%s' is running successfully\n", current->label);
            Serial.println("Clearing boot tracking");
            
            // Clear all tracking - app is confirmed working
            prefs.clear();
        }
        
        prefs.end();
    }
    
    /**
     * Prepare to load a partition. Sets up tracking.
     * Call this before switching partitions.
     */
    void prepareLoad(const char* partitionLabel) {
        prefs.begin(BOOT_CHECK_NAMESPACE, false);
        
        // Reset tracking for new boot
        prefs.putUInt(BOOT_COUNT_KEY, 0);
        prefs.putULong(BOOT_TIME_KEY, millis());
        prefs.putString(TARGET_PARTITION_KEY, partitionLabel);
        
        Serial.println("\n=== Preparing Partition Load ===");
        Serial.printf("Target partition: %s\n", partitionLabel);
        Serial.println("Boot tracking initialized");
        
        prefs.end();
    }
    
    /**
     * Return to factory partition immediately.
     */
    void returnToFactory() {
        Serial.println("\n=== Returning to Factory ===");
        
        const esp_partition_t* factory = esp_partition_find_first(
            ESP_PARTITION_TYPE_APP,
            ESP_PARTITION_SUBTYPE_ANY,
            "factory"
        );
        
        if (factory != NULL) {
            // Clear tracking
            prefs.begin(BOOT_CHECK_NAMESPACE, false);
            prefs.clear();
            prefs.end();
            
            // Set factory as boot partition
            esp_err_t err = esp_ota_set_boot_partition(factory);
            if (err == ESP_OK) {
                Serial.println("Factory partition set. Rebooting...");
                Serial.flush();
                delay(1000);
                esp_restart();
            } else {
                Serial.printf("ERROR: Failed to set factory partition (code %d)\n", err);
            }
        } else {
            Serial.println("ERROR: Factory partition not found!");
        }
    }
    
    /**
     * Cancel any pending return to factory.
     * Clears all tracking.
     */
    void cancelReturn() {
        prefs.begin(BOOT_CHECK_NAMESPACE, false);
        prefs.clear();
        prefs.end();
        
        Serial.println("Return to factory cancelled - tracking cleared");
    }
};

#endif // PARTITION_MANAGER_H
