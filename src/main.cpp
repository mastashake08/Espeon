#include <Arduino.h>
#include <heltec_unofficial.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"

// Button configuration
#define PRG_BUTTON 0
#define DEBOUNCE_DELAY 50
#define DOUBLE_CLICK_DELAY 300
#define LONG_PRESS_DELAY 2000

// Menu structure
struct AppPartition {
  const char* label;
  const char* name;
  const char* description;
};

// Define available application partitions
const AppPartition appPartitions[] = {
  {"Meshtastic", "Meshtastic", "Mesh Network"},
  {"LoRABLE", "LoRABLE", "LoRa + BLE App"}
};
const int numPartitions = sizeof(appPartitions) / sizeof(AppPartition);

// Menu state
int selectedIndex = 0;
bool needsRedraw = true;

// Button state
enum ButtonState { IDLE, PRESSED, RELEASED, SINGLE_CLICK, DOUBLE_CLICK, LONG_PRESS };
ButtonState buttonState = IDLE;
unsigned long buttonPressTime = 0;
unsigned long buttonReleaseTime = 0;
int clickCount = 0;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;

// Forward declaration
void loadPartition(const char* label);

void drawMenu() {
  display.clear();
  
  // Title
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Factory Menu");
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
  display.drawString(0, 52, "Click:Nav  2x:Load");
  
  display.display();
  needsRedraw = false;
}

void handleButtonPress() {
  bool reading = digitalRead(PRG_BUTTON) == LOW;
  unsigned long currentTime = millis();
  
  // Debounce
  if (reading != lastButtonState) {
    lastDebounceTime = currentTime;
  }
  
  if ((currentTime - lastDebounceTime) > DEBOUNCE_DELAY) {
    // Button pressed
    if (reading && buttonState == IDLE) {
      buttonState = PRESSED;
      buttonPressTime = currentTime;
    }
    
    // Check for long press
    if (reading && buttonState == PRESSED) {
      if ((currentTime - buttonPressTime) > LONG_PRESS_DELAY) {
        buttonState = LONG_PRESS;
        // Handle long press (optional feature)
        Serial.println("Long press detected");
      }
    }
    
    // Button released
    if (!reading && buttonState == PRESSED) {
      buttonState = RELEASED;
      buttonReleaseTime = currentTime;
      clickCount++;
    }
  }
  
  lastButtonState = reading;
  
  // Process click events
  if (buttonState == RELEASED) {
    if ((currentTime - buttonReleaseTime) > DOUBLE_CLICK_DELAY) {
      if (clickCount == 1) {
        buttonState = SINGLE_CLICK;
      } else if (clickCount >= 2) {
        buttonState = DOUBLE_CLICK;
      }
      clickCount = 0;
    }
  }
  
  // Handle single click - navigate menu
  if (buttonState == SINGLE_CLICK) {
    selectedIndex = (selectedIndex + 1) % numPartitions;
    needsRedraw = true;
    Serial.printf("Selected: %s\n", appPartitions[selectedIndex].name);
    buttonState = IDLE;
  }
  
  // Handle double click - load partition
  if (buttonState == DOUBLE_CLICK) {
    Serial.printf("Loading partition: %s\n", appPartitions[selectedIndex].label);
    loadPartition(appPartitions[selectedIndex].label);
    buttonState = IDLE;
  }
  
  // Reset long press
  if (buttonState == LONG_PRESS && !reading) {
    buttonState = IDLE;
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
  
  // Set boot partition and restart
  err = esp_ota_set_boot_partition(partition);
  if (err == ESP_OK) {
    Serial.println("Rebooting to new partition...");
    display.clear();
    display.drawString(20, 25, "Rebooting...");
    display.display();
    delay(1000);
    esp_restart();
  } else {
    Serial.println("Failed to set boot partition!");
    display.clear();
    display.drawString(15, 25, "Load Failed!");
    display.display();
    delay(2000);
    needsRedraw = true;
  }
}

void setup() {
  // Initialize serial
  Serial.begin(115200);
  delay(100);
  Serial.println("\n=== Factory Menu Application ===");
  
  // Initialize Heltec board
  heltec_setup();
  
  // Initialize PRG button
  pinMode(PRG_BUTTON, INPUT_PULLUP);
  
  // Display welcome screen
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.drawString(35, 15, "Espeon");
  display.setFont(ArialMT_Plain_10);
  display.drawString(10, 40, "Factory Menu v1.0");
  display.display();
  delay(2000);
  
  // Print current partition info
  const esp_partition_t* current = esp_ota_get_running_partition();
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
  
  Serial.println("\nMenu Controls:");
  Serial.println("  Single Click: Navigate");
  Serial.println("  Double Click: Load Partition");
  
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