#include <Arduino.h>
#include <heltec_unofficial.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include <Wire.h>
#include <Preferences.h>

// Button configuration
#define PRG_BUTTON 0
#define DEBOUNCE_DELAY 50
#define DOUBLE_CLICK_WINDOW 400
#define LONG_PRESS_DELAY 3000  // 3 seconds for returning to menu

// Display control
#define VEXT_CTRL 36  // Display power control
#define OLED_SDA 17
#define OLED_SCL 18

// NVS keys for boot management
#define BOOT_NAMESPACE "espeon"
#define CURRENT_APP_KEY "current_app"
#define RETURN_REQUEST_KEY "return_req"

// RTC memory to track resets (persists across reboots)
RTC_DATA_ATTR uint32_t rtcBootCount = 0;
RTC_DATA_ATTR uint64_t rtcLastBootTime = 0;

// Menu structure
struct AppPartition {
  const char* label;
  const char* name;
  const char* description;
};

// Define available application partitions
const AppPartition appPartitions[] = {
  {"Meshtastic", "Meshtastic", "Mesh Network"},
  {"LoRABLE", "LoRABLE", "LoRa + BLE App"},
  {"WifiAP", "WifiAP", "WiFi Access Point"}
};
const int numPartitions = sizeof(appPartitions) / sizeof(AppPartition);

// Menu state
int selectedIndex = 0;
bool needsRedraw = true;
bool showingConfirmation = false;

// Button state - simplified approach
unsigned long lastPressTime = 0;
unsigned long lastReleaseTime = 0;
unsigned long pressStartTime = 0;
bool wasPressed = false;
bool waitingForSecondClick = false;
int clickCount = 0;

// Forward declaration
void loadPartition(const char* label);

void drawMenu() {
  display.clear();
  
  // Title
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Espeon Loader");
  display.drawHorizontalLine(0, 12, 128);
  
  // Battery voltage
  char batteryStr[16];
  float voltage = heltec_vbat();
  snprintf(batteryStr, sizeof(batteryStr), "%.2fV", voltage);
  display.drawString(90, 0, batteryStr);
  
  // Menu items
  int yPos = 16;
  
  for (int i = 0; i < numPartitions && i < 4; i++) {
    int displayIndex = (selectedIndex / 4) * 4 + i;
    if (displayIndex >= numPartitions) break;
    
    // Highlight selected item
    if (displayIndex == selectedIndex) {
      display.setColor(WHITE);
      display.fillRect(0, yPos, 128, 11);
      display.setColor(BLACK);
      display.drawString(2, yPos, appPartitions[displayIndex].name);
      display.setColor(WHITE);
    } else {
      display.drawString(2, yPos, appPartitions[displayIndex].name);
    }
    
    yPos += 12;
  }
  
  // Instructions at bottom
  display.setFont(ArialMT_Plain_10);
  if (showingConfirmation) {
    display.drawString(0, 52, "Click:Cancel 2x:OK");
  } else {
    display.drawString(0, 52, "Click:Nav  2x:Load");
  }
  
  display.display();
  needsRedraw = false;
}

void showConfirmation() {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  
  // Title
  display.drawString(0, 0, "Load App?");
  display.drawHorizontalLine(0, 12, 128);
  
  // Selected app name
  display.setFont(ArialMT_Plain_16);
  int width = display.getStringWidth(appPartitions[selectedIndex].name);
  int x = (128 - width) / 2;
  display.drawString(x, 20, appPartitions[selectedIndex].name);
  
  // Instructions
  display.setFont(ArialMT_Plain_10);
  display.drawString(5, 42, "Double-click: Confirm");
  display.drawString(5, 54, "Single-click: Cancel");
  
  display.display();
}

void handleButtonPress() {
  bool isPressed = (digitalRead(PRG_BUTTON) == LOW);
  unsigned long currentTime = millis();
  
  // Detect button press (not pressed -> pressed)
  if (isPressed && !wasPressed) {
    pressStartTime = currentTime;
    wasPressed = true;
    Serial.println("Button pressed");
  }
  
  // Detect button release (pressed -> not pressed)
  if (!isPressed && wasPressed) {
    unsigned long pressDuration = currentTime - pressStartTime;
    wasPressed = false;
    
    // Check for long press
    if (pressDuration > LONG_PRESS_DELAY) {
      Serial.println("Long press detected");
      // Optional: Add long press functionality
      return;
    }
    
    // Regular click detected
    lastReleaseTime = currentTime;
    clickCount++;
    Serial.printf("Click registered (count: %d)\n", clickCount);
    
    if (clickCount == 1) {
      // First click - start waiting for potential second click
      waitingForSecondClick = true;
      Serial.println("Waiting for second click...");
    }
  }
  
  // Check if we're waiting for a second click
  if (waitingForSecondClick && (currentTime - lastReleaseTime > DOUBLE_CLICK_WINDOW)) {
    // Timeout - it was a single click
    if (clickCount == 1) {
      Serial.println("Single click confirmed");
      
      if (showingConfirmation) {
        // Cancel confirmation
        showingConfirmation = false;
        needsRedraw = true;
        Serial.println("Confirmation cancelled");
      } else {
        // Navigate menu
        selectedIndex = (selectedIndex + 1) % numPartitions;
        needsRedraw = true;
        Serial.printf("Selected: %s\n", appPartitions[selectedIndex].name);
      }
    }
    
    clickCount = 0;
    waitingForSecondClick = false;
  }
  
  // Check for double click
  if (clickCount >= 2) {
    Serial.println("Double click detected!");
    
    if (showingConfirmation) {
      // Confirm and load partition
      Serial.printf("Loading partition: %s\n", appPartitions[selectedIndex].label);
      showingConfirmation = false;
      loadPartition(appPartitions[selectedIndex].label);
    } else {
      // Show confirmation
      showingConfirmation = true;
      showConfirmation();
      Serial.println("Showing confirmation dialog");
    }
    
    clickCount = 0;
    waitingForSecondClick = false;
  }
}

void loadPartition(const char* label) {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(30, 25, "Loading...");
  display.display();
  
  // Find the partition
  const esp_partition_t* partition = esp_partition_find_first(
    ESP_PARTITION_TYPE_APP,
    ESP_PARTITION_SUBTYPE_ANY,
    label
  );
  
  if (partition == NULL) {
    Serial.printf("Partition '%s' not found!\n", label);
    display.clear();
    display.drawString(20, 20, "Partition");
    display.drawString(20, 35, "Not Found!");
    display.display();
    delay(2000);
    needsRedraw = true;
    return;
  }
  
  // Validate partition
  esp_app_desc_t app_desc;
  esp_err_t err = esp_ota_get_partition_description(partition, &app_desc);
  
  if (err != ESP_OK) {
    Serial.println("Invalid partition data!");
    display.clear();
    display.drawString(25, 20, "Invalid");
    display.drawString(20, 35, "Partition!");
    display.display();
    delay(2000);
    needsRedraw = true;
    return;
  }
  
  // Save this as the current/default app in NVS
  Preferences prefs;
  prefs.begin(BOOT_NAMESPACE, false);
  prefs.putString(CURRENT_APP_KEY, label);
  prefs.putUInt("boot_count", 0);  // Reset boot counter
  prefs.end();
  
  Serial.printf("Saved '%s' as default app\n", label);
  Serial.println("Tip: Reset 3 times quickly to return to factory menu");
  
  // Set boot partition and restart
  Serial.printf("Setting boot partition to: %s (subtype: 0x%x)\n", partition->label, partition->subtype);
  err = esp_ota_set_boot_partition(partition);
  if (err == ESP_OK) {
    Serial.println("Boot partition set successfully!");
    Serial.println("Rebooting to new partition...");
    display.clear();
    display.drawString(20, 25, "Rebooting...");
    display.display();
    delay(1000);
    Serial.println("Calling esp_restart() now...");
    Serial.flush();
    esp_restart();
    // Should never reach here
    Serial.println("ERROR: esp_restart() returned!");
  } else {
    Serial.printf("Failed to set boot partition! Error: 0x%x\n", err);
    display.clear();
    display.drawString(15, 20, "Load Failed!");
    display.setFont(ArialMT_Plain_10);
    char errMsg[32];
    snprintf(errMsg, sizeof(errMsg), "Error: 0x%x", err);
    display.drawString(15, 35, errMsg);
    display.display();
    delay(3000);
    needsRedraw = true;
  }
}

void setup() {
  // Initialize serial
  Serial.begin(115200);
  delay(100);
  Serial.println("\n=== Factory Menu Application ===");
  
  // Initialize PRG button FIRST (before anything else)
  pinMode(PRG_BUTTON, INPUT_PULLUP);
  delay(50);
  
  // Initialize display EARLY so we can show boot status
  pinMode(VEXT_CTRL, OUTPUT);
  digitalWrite(VEXT_CTRL, LOW);  // Power on display
  delay(100);
  heltec_setup();  // This initializes I2C, display, and other peripherals
  delay(100);
  
  // Show early boot message
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Espeon Booting...");
  display.display();
  
  // Check for PRG button held during boot (force return to factory)
  bool buttonHeld = (digitalRead(PRG_BUTTON) == LOW);
  
  // Initialize Preferences for boot management
  Preferences prefs;
  prefs.begin(BOOT_NAMESPACE, false);
  
  // Use RTC memory to track rapid resets (survives soft reset)
  uint64_t currentBootTime = esp_timer_get_time() / 1000000;  // Convert to seconds
  bool rapidReset = false;
  
  // Check if this boot happened within 10 seconds of last boot
  if (rtcLastBootTime > 0 && (currentBootTime - rtcLastBootTime) < 10) {
    rtcBootCount++;
    rapidReset = true;
    Serial.printf("Rapid reset detected! Count: %d (within %llu seconds)\n", 
                  rtcBootCount, currentBootTime - rtcLastBootTime);
  } else {
    // More than 10 seconds since last boot - reset counter
    rtcBootCount = 1;
    rapidReset = false;
    Serial.println("Normal boot - reset counter cleared");
  }
  
  rtcLastBootTime = currentBootTime;
  
  // Check if user is requesting return to factory (3+ rapid resets OR button held)
  bool forceFactoryMode = (rtcBootCount >= 3) || buttonHeld;
  
  if (forceFactoryMode) {
    if (buttonHeld) {
      Serial.println("\n!!! PRG BUTTON HELD - FORCING FACTORY MODE !!!");
    }
    if (rtcBootCount >= 3) {
      Serial.printf("\n!!! %d RAPID RESETS DETECTED - FORCING FACTORY MODE !!!\n", rtcBootCount);
    }
    
    // Clear counters and stay in factory
    rtcBootCount = 0;
    prefs.putString(CURRENT_APP_KEY, "");  // Clear any default app
    prefs.end();
    
    // Continue with factory menu initialization below...
  } else {
    // Check if there's a default app to auto-load
    String defaultApp = prefs.getString(CURRENT_APP_KEY, "");
    prefs.end();
    
    if (defaultApp.length() > 0 && rtcBootCount < 3) {
      Serial.printf("Auto-loading default app: %s\n", defaultApp.c_str());
      Serial.println("(Reset 3 times quickly to return to menu)");
      
      // Show on display
      display.clear();
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 10, "Auto-loading:");
      display.setFont(ArialMT_Plain_16);
      display.drawString(0, 25, defaultApp.c_str());
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 45, "Hold PRG to cancel");
      display.display();
      
      // Give user 2 seconds to see the message, then load app
      delay(2000);
      
      // Load the default app
      const esp_partition_t* partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP,
        ESP_PARTITION_SUBTYPE_ANY,
        defaultApp.c_str()
      );
      
      if (partition != NULL) {
        // Validate that the partition contains a valid app
        esp_app_desc_t app_desc;
        esp_err_t err = esp_ota_get_partition_description(partition, &app_desc);
        
        if (err == ESP_OK) {
          // Valid app found, boot into it
          esp_ota_set_boot_partition(partition);
          Serial.println("Rebooting to default app...");
          display.clear();
          display.drawString(20, 25, "Rebooting...");
          display.display();
          delay(500);
          esp_restart();
        } else {
          // Invalid or empty partition - clear default and stay in menu
          Serial.println("ERROR: Partition exists but contains no valid app!");
          Serial.println("Clearing default app and staying in factory menu");
          display.clear();
          display.drawString(0, 15, "ERROR: Invalid");
          display.drawString(0, 30, "partition!");
          display.drawString(0, 45, "Clearing & staying");
          display.display();
          prefs.begin(BOOT_NAMESPACE, false);
          prefs.putString(CURRENT_APP_KEY, "");  // Clear invalid default
          prefs.end();
          delay(2000);
        }
      } else {
        Serial.println("ERROR: Default app partition not found!");
        Serial.println("Clearing default app and staying in factory menu");
        display.clear();
        display.drawString(0, 20, "ERROR: Partition");
        display.drawString(0, 35, "not found!");
        display.display();
        prefs.begin(BOOT_NAMESPACE, false);
        prefs.putString(CURRENT_APP_KEY, "");  // Clear invalid default
        prefs.end();
        delay(2000);
      }
    }
  }
  
  // If we get here, we're staying in factory menu
  rtcBootCount = 0;  // Reset RTC counter when staying in menu
  prefs.begin(BOOT_NAMESPACE, false);
  prefs.end();
  
  // Get current running partition
  const esp_partition_t* current = esp_ota_get_running_partition();
  
  // Display welcome screen
  Serial.println("Displaying welcome screen...");
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.drawString(35, 15, "Espeon");
  display.setFont(ArialMT_Plain_10);
  display.drawString(10, 40, "Factory Menu v1.0");
  display.display();
  delay(2000);
  
  // Print current partition info
  Serial.printf("Running from partition: %s\n", current->label);
  
  // Print available partitions
  Serial.println("\nAvailable partitions:");
  esp_partition_iterator_t it = esp_partition_find(
    ESP_PARTITION_TYPE_APP,
    ESP_PARTITION_SUBTYPE_ANY,
    NULL
  );
  
  while (it != NULL) {
    const esp_partition_t* part = esp_partition_get(it);
    Serial.printf("  - %s (0x%x, %d bytes)\n", 
                  part->label, part->address, part->size);
    it = esp_partition_next(it);
  }
  esp_partition_iterator_release(it);
  
  Serial.println("\n==================================");
  Serial.println("Menu Controls:");
  Serial.println("  Single Click: Navigate");
  Serial.println("  Double Click: Load Partition");
  Serial.println("\nReturn to Factory Menu:");
  Serial.println("  Hold PRG during boot/reset");
  Serial.println("==================================\n");
  
  needsRedraw = true;
}

void loop() {
  heltec_loop();
  
  // Handle button input
  handleButtonPress();
  
  // Update display if needed
  if (needsRedraw) {
    drawMenu();
  }
  
  delay(10);
}