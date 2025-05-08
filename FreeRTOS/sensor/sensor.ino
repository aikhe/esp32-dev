/**
 * ESP32 HC-SR04 Distance Sensor with FreeRTOS
 *
 * This program reads distance from HC-SR04 ultrasonic sensor
 * and controls 3 LEDs based on the measured distance using FreeRTOS.
 * LEDs and LCD turn off automatically after 10 minutes unless new distance level is detected.
 * Includes a 5-second debounce to prevent rapid LED state changes.
 */

#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// HC-SR04 Sensor pins
#define TRIG_PIN 17
#define ECHO_PIN 16

// LED pins
#define LED_ONE 13
#define LED_TWO 12
#define LED_THREE 14

// Constants
#define SOUND_SPEED 0.034  // Sound speed in cm/uS
#define DISTANCE_READ_INTERVAL 100  // ms
#define LED_UPDATE_INTERVAL 50      // ms
#define LCD_UPDATE_INTERVAL 500     // ms
#define LED_TIMEOUT_MINUTES 10      // LED stays on for 10 minutes
#define LED_TIMEOUT_MS (LED_TIMEOUT_MINUTES * 60 * 1000)  // 10 minutes in milliseconds
#define DEBOUNCE_TIME_MS 5000       // 5 seconds debounce to prevent rapid changes

// Global variables for sharing data between tasks
volatile float currentDistance = 0;
SemaphoreHandle_t distanceMutex;

// LCD Setup - Assuming standard 16x2 I2C LCD at address 0x27
// Adjust address if your LCD uses a different one
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Timeout and state management - shared between LED and LCD
unsigned long stateLastChangeTime = 0;  // Shared timer for both LED and LCD
unsigned long lastStateUpdateTime = 0;  // For debounce
int lastDetectedRange = 0;  // 0=no detection, 1=far, 2=medium, 3=close
int activeState = 0;        // Current active state for both LED and LCD
bool timeoutEnabled = true;

// Task function prototypes
void readDistanceTask(void *parameter);
void controlOutputsTask(void *parameter);

/**
 * Task to read distance from HC-SR04 sensor
 */
void readDistanceTask(void *parameter) {
  float distance;
 
  while(true) {
    // Clears the TRIG_PIN
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    
    // Sets the TRIG_PIN HIGH for 10 microseconds
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    // Reads the ECHO_PIN, returns the sound wave travel time in microseconds
    float duration = pulseIn(ECHO_PIN, HIGH);
    
    // Calculate the distance
    distance = duration * SOUND_SPEED / 2;
    
    // Print the distance on the Serial Monitor
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
    
    // Update the shared distance variable with mutex protection
    if (xSemaphoreTake(distanceMutex, portMAX_DELAY) == pdTRUE) {
      currentDistance = distance;
      xSemaphoreGive(distanceMutex);
    }
    
    // Wait before next reading
    vTaskDelay(DISTANCE_READ_INTERVAL / portTICK_PERIOD_MS);
  }
}

/**
 * Combined task to control both LEDs and LCD based on the measured distance
 */
void controlOutputsTask(void *parameter) {
  float distance;
  int currentRange = 0;  // Current detected distance range
  unsigned long currentTime;
  String statusMessage = "";
 
  while(true) {
    currentTime = millis();
    
    // Get the current distance with mutex protection
    if (xSemaphoreTake(distanceMutex, portMAX_DELAY) == pdTRUE) {
      distance = currentDistance;
      xSemaphoreGive(distanceMutex);
    }
    
    // Determine current range based on distance
    // Only consider valid levels (no "normal" state)
    if (distance <= 15) {
      currentRange = 3;  // Close range - Warning
    } else if (distance <= 25) {
      currentRange = 2;  // Medium range - Critical
    } else if (distance <= 40) {
      currentRange = 1;  // Far range - Alert
    } else {
      currentRange = 0;  // Out of range - ignore
    }
    
    // Check if a valid range is detected and if debounce period has passed
    if (currentRange != lastDetectedRange && currentRange > 0 && 
        (currentTime - lastStateUpdateTime) >= DEBOUNCE_TIME_MS) {
      // New valid range detected and debounce time passed
      lastDetectedRange = currentRange;
      lastStateUpdateTime = currentTime; // Update debounce timer
      stateLastChangeTime = currentTime; // Reset 10-minute timer
      activeState = currentRange;
      
      // Update LEDs based on new range
      digitalWrite(LED_ONE, currentRange == 1 ? HIGH : LOW);
      digitalWrite(LED_TWO, currentRange == 2 ? HIGH : LOW);
      digitalWrite(LED_THREE, currentRange == 3 ? HIGH : LOW);
      
      // Determine status message for LCD
      switch(currentRange) {
        case 1:
          statusMessage = "Alert";
          break;
        case 2:
          statusMessage = "Critical";
          break;
        case 3:
          statusMessage = "Warning";
          break;
      }
      
      // Update LCD with new information
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Water Dis: ");
      lcd.print((int)distance);
      lcd.print("cm");
      
      lcd.setCursor(0, 1);
      lcd.print("Status: ");
      lcd.print(statusMessage);
      
      Serial.print("New range detected: ");
      Serial.println(currentRange);
      Serial.print("Status: ");
      Serial.println(statusMessage);
      Serial.println("Timer reset to 10 minutes");
    } else if (currentRange != lastDetectedRange && currentRange > 0) {
      // Range changed but within debounce period - ignore the change
      Serial.print("Ignoring change to range ");
      Serial.print(currentRange);
      Serial.println(" - debounce period active");
    }
    
    // Check if timer has expired and outputs should be turned off
    if (timeoutEnabled && activeState > 0 && (currentTime - stateLastChangeTime) >= LED_TIMEOUT_MS) {
      // Turn off all LEDs
      digitalWrite(LED_ONE, LOW);
      digitalWrite(LED_TWO, LOW);
      digitalWrite(LED_THREE, LOW);
      
      // Clear LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Monitoring...");
      lcd.setCursor(0, 1);
      lcd.print("Ready");
      
      // Update state
      activeState = 0;
      
      Serial.println("Timeout reached (10 minutes) - All outputs turned off");
    }
    
    // Wait before next update
    vTaskDelay(LED_UPDATE_INTERVAL / portTICK_PERIOD_MS);
  }
}

void setup() {
  // Initialize Serial communication
  Serial.begin(115200);
  
  // Initialize LCD
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Water Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(1000);
 
  // Configure HC-SR04 pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
 
  // Configure LED pins
  pinMode(LED_ONE, OUTPUT);
  pinMode(LED_TWO, OUTPUT);
  pinMode(LED_THREE, OUTPUT);
 
  // Initially turn off all LEDs
  digitalWrite(LED_ONE, LOW);
  digitalWrite(LED_TWO, LOW);
  digitalWrite(LED_THREE, LOW);
 
  // Create mutex for shared data protection
  distanceMutex = xSemaphoreCreateMutex();
 
  // Create FreeRTOS tasks
  xTaskCreate(
    readDistanceTask,     // Task function
    "ReadDistance",       // Task name
    2048,                 // Stack size (bytes)
    NULL,                 // Task parameters
    1,                    // Priority (1 is low)
    NULL                  // Task handle
  );
 
  xTaskCreate(
    controlOutputsTask,   // Task function to control both LEDs and LCD
    "ControlOutputs",     // Task name
    2048,                 // Stack size (bytes)
    NULL,                 // Task parameters
    1,                    // Priority
    NULL                  // Task handle
  );
 
  Serial.println("ESP32 HC-SR04 Distance Sensor with FreeRTOS Started");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Monitoring...");
  lcd.setCursor(0, 1);
  lcd.print("Ready");
}

void loop() {
  // Empty loop as tasks are handling everything
  vTaskDelay(1000 / portTICK_PERIOD_MS); // Just keep the loop alive
}
