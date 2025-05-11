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
#include "Arduino.h"
#include "Audio.h"
#include "SD.h"
#include "FS.h"
#include "SPI.h"

const char *ssid = "TK-gacura";
const char *password = "gisaniel924";

// HC-SR04 Sensor pins
#define TRIG_PIN 17
#define ECHO_PIN 16

// LED pins
#define LED_ONE 13
#define LED_TWO 12
#define LED_THREE 14

// microSD Card Reader connections
#define SD_CS          5
#define SPI_MOSI      23 
#define SPI_MISO      19
#define SPI_SCK       18

// I2S Connections (MAX98357)
#define I2S_DOUT      25
#define I2S_BCLK      27
#define I2S_LRC       26

// Constants
#define SOUND_SPEED 0.034  // Sound speed in cm/uS
#define DISTANCE_READ_INTERVAL 100  // ms
#define LED_UPDATE_INTERVAL 50      // ms
#define LCD_UPDATE_INTERVAL 4000    // ms - reduced for more responsive updates
#define LED_TIMEOUT_MINUTES 10      // LED stays on for 10 minutes
#define LED_TIMEOUT_MS (LED_TIMEOUT_MINUTES * 60 * 1000)  // 10 minutes in milliseconds
#define DEBOUNCE_TIME_MS 5000       // 5 seconds debounce to prevent rapid changes
#define LCD_LOCK_TIME_MS 5000       // 5 seconds to lock LCD text after level detection

// Global variables for sharing data between tasks
volatile float currentDistance = 0;
SemaphoreHandle_t distanceMutex;

// Create Audio object
Audio audio;

// LCD Setup - Assuming standard 16x2 I2C LCD at address 0x27
// Adjust address if your LCD uses a different one
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Timeout and state management - shared between LED and LCD
unsigned long stateLastChangeTime = 0;  // Shared timer for both LED and LCD
unsigned long lastStateUpdateTime = 0;  // For debounce
int lastDetectedRange = 0;  // 0=no detection, 1=far, 2=medium, 3=close
int activeState = 0;        // Current active state for both LED and LCD
bool timeoutEnabled = true;
unsigned long lastLCDUpdateTime = 0;  // Track last LCD update time
// Track last status display to avoid unnecessary updates
String currentDisplayedStatus = "";
// Variables for LCD text locking
unsigned long lcdLockStartTime = 0;
bool lcdTextLocked = false;

// Weather and location information (static strings)
const String location = "New York";
const String weather = "Sunny";
const String temperature = "25C";

// Task function prototypes
void readDistanceTask(void *parameter);
void controlOutputsTask(void *parameter);
void audioTask(void *parameter);

// Task handle for audio playback
TaskHandle_t audioTaskHandle;

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
      statusMessage = "Warning";
    } else if (distance <= 25) {
      currentRange = 2;  // Medium range - Critical
      statusMessage = "Critical";
    } else if (distance <= 40) {
      currentRange = 1;  // Far range - Alert
      statusMessage = "Alert";
    } else {
      currentRange = 0;  // Out of range - ignore
      statusMessage = "Normal";
    }
    
    // Check if LCD text lock should be released
    if (lcdTextLocked && (currentTime - lcdLockStartTime) >= LCD_LOCK_TIME_MS) {
      lcdTextLocked = false;
      Serial.println("LCD text lock released");
      
      // When LCD lock is released, return to showing location/weather
      // but keep LEDs on until timeout
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Loc: ");
      lcd.print(location);
      
      lcd.setCursor(0, 1);
      lcd.print(weather);
      lcd.print(" ");
      lcd.print(temperature);
      
      lastLCDUpdateTime = currentTime; // Reset LCD update timer
    }
    
    // Update LCD with current information
    if (!lcdTextLocked && currentTime - lastLCDUpdateTime >= LCD_UPDATE_INTERVAL) {
      // Clear LCD for clean display
      lcd.clear();
      
      // Always show location and weather on LCD when not locked
      lcd.setCursor(0, 0);
      lcd.print("Loc: ");
      lcd.print(location);
      
      lcd.setCursor(0, 1);
      lcd.print(weather);
      lcd.print(" ");
      lcd.print(temperature);
      currentDisplayedStatus = "Normal";
      
      lastLCDUpdateTime = currentTime;
    }
    
    // Check if a valid range is detected and if debounce period has passed
    if (currentRange != lastDetectedRange && 
        (currentTime - lastStateUpdateTime) >= DEBOUNCE_TIME_MS) {
      // New valid range detected and debounce time passed
      lastDetectedRange = currentRange;
      lastStateUpdateTime = currentTime; // Update debounce timer
      
      if (currentRange > 0) {
            case 1:  // Alert (far)
              Serial.println("Playing alert sound");
              if (!audio.isRunning()) {  // Only start a new file if not already playing
                audio.connecttoFS(SD, "/LOW-FLOOD-HIGH.mp3");
              }
              break;
            case 2:  // Critical (medium)
              Serial.println("Playing critical sound");
              if (!audio.isRunning()) {  // Only start a new file if not already playing
                audio.connecttoFS(SD, "/MEDIUM-FLOOD-HIGH2.mp3");
              }
              break;
            case 3:  // Warning (close)
              Serial.println("Playing warning sound");
              if (!audio.isRunning()) {  // Only start a new file if not already playing
                audio.connecttoFS(SD, "/HIGH-FLOOD-HIGH2.mp3");
              }
              break;
          }
        } else {
          Serial.println("Alert sound files not found on SD card");
        }
        
        // Lock the LCD text for 5 seconds
        lcdTextLocked = true;
        lcdLockStartTime = currentTime;
        
        // Force update the LCD immediately with the water level info
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Water Dis: ");
        lcd.print((int)distance);
        lcd.print("cm");
        
        lcd.setCursor(0, 1);
        lcd.print("Status: ");
        lcd.print(statusMessage);
        currentDisplayedStatus = statusMessage;
        
        Serial.print("New range detected: ");
        Serial.println(currentRange);
        Serial.print("Status: ");
        Serial.println(statusMessage);
        Serial.println("Timer reset to 10 minutes");
        Serial.println("LCD text locked for 5 seconds");
      } else {
        // No water level detected
        // Only turn off LEDs if no active state or timeout has expired
        if (activeState == 0 || (currentTime - stateLastChangeTime) >= LED_TIMEOUT_MS) {
          digitalWrite(LED_ONE, LOW);
          digitalWrite(LED_TWO, LOW);
          digitalWrite(LED_THREE, LOW);
          activeState = 0;
        }
        
        // Update LCD with location and weather
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Loc: ");
        lcd.print(location);
        
        lcd.setCursor(0, 1);
        lcd.print(weather);
        lcd.print(" ");
        lcd.print(temperature);
        
        Serial.println("No water level detected");
      }
    } else if (currentRange != lastDetectedRange) {
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
      
      // Update LCD with location and weather if not already showing
      if (currentDisplayedStatus != "Normal") {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Loc: ");
        lcd.print(location);
        
        lcd.setCursor(0, 1);
        lcd.print(weather);
        lcd.print(" ");
        lcd.print(temperature);
        
        currentDisplayedStatus = "Normal";
      }
      
      // Update state
      activeState = 0;
      lcdTextLocked = false; // Make sure lock is released
      
      Serial.println("Timeout reached (10 minutes) - All outputs turned off");
    }
    
    // Wait before next update
    vTaskDelay(LED_UPDATE_INTERVAL / portTICK_PERIOD_MS);
  }
}

// Audio playback task with improved error handling
void audioTask(void *parameter) {
  // Add a short delay before starting audio processing
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  
  while (true) {
    // Use try-catch pattern with a flag to handle potential errors
    bool audioProcessed = false;
    
    // Process audio with error protection
    try {
      audio.loop();
      audioProcessed = true;
    } catch (...) {
      Serial.println("Error in audio processing");
    }
    
    // If audio processing failed, give more time before retry
    if (!audioProcessed) {
      vTaskDelay(500 / portTICK_PERIOD_MS);
    } else {
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  }
}

void setup() {
  // Initialize Serial communication
  Serial.begin(115200);
  delay(100); // Short delay for serial stability

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(1000);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    Serial.print("IP Address: ");
  }
  
  Serial.println("\n\n--- Starting Water Monitor System ---");
  
  // Initialize LCD
  Wire.begin();
  delay(50); // Short delay for I2C stability
  
  // Initialize LCD - using the proper initialization sequence
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
  digitalWrite(TRIG_PIN, LOW); // Ensure trigger starts LOW
 
  // Configure LED pins
  pinMode(LED_ONE, OUTPUT);
  pinMode(LED_TWO, OUTPUT);
  pinMode(LED_THREE, OUTPUT);
  
  // Turn off all LEDs initially
  digitalWrite(LED_ONE, LOW);
  digitalWrite(LED_TWO, LOW);
  digitalWrite(LED_THREE, LOW);
  
  // Create mutex for shared data protection
  distanceMutex = xSemaphoreCreateMutex();
  if (distanceMutex == NULL) {
    Serial.println("Failed to create mutex!");
    while(1); // Critical failure
  }
  
  // Set microSD Card CS pin
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  delay(100); // Give SD card time to initialize
  
  // Initialize SPI for SD card
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  
  // Initialize SD card with retry
  bool sdInitialized = false;
  for (int retry = 0; retry < 3 && !sdInitialized; retry++) {
    if (SD.begin(SD_CS, SPI)) {
      sdInitialized = true;
      Serial.println("SD card initialized.");
      
      // Check if audio files exist
      bool filesExist = SD.exists("/DEVICE-START-VOICE.mp3") && 
                        SD.exists("/LOW-FLOOD-HIGH.mp3") && 
                        SD.exists("/MEDIUM-FLOOD-HIGH2.mp3") && 
                        SD.exists("/HIGH-FLOOD-HIGH2.mp3");

      /DEVICE-START-VOICE.mp3
      /LOW-FLOOD-HIGH.mp3
      /MEDIUM-FLOOD-HIGH2.mp3
      /HIGH-FLOOD-HIGH2.mp3
      
      if (filesExist) {
        // Set up I2S for audio output
        audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
        
        // Set volume level (0–100)
        audio.setVolume(80); // Slightly lower volume for stability
        
        delay(100); // Short delay before playing audio
        
        // Play the initial audio file
        if (SD.exists("/DEVICE-START-VOICE.mp3")) {
          audio.connecttoFS(SD, "/DEVICE-START-VOICE.mp3");
          
          // Create FreeRTOS task for audio playback with higher stack size
          xTaskCreate(
            audioTask,           // Task function
            "AudioTask",         // Name
            8192,                // Stack size - increased for stability
            NULL,                // Parameter
            1,                   // Priority
            &audioTaskHandle     // Task handle
          );
        }
      } else {
        Serial.println("Required audio files not found on SD card!");
      }
    } else {
      Serial.println("SD card initialization failed! Retrying...");
      delay(500);
    }
  }
  
  if (!sdInitialized) {
    Serial.println("SD card initialization failed after retries. Continuing without audio.");
  }

  // CHECKPOINT
 
  // Create FreeRTOS tasks with increased stack sizes
  xTaskCreate(
    readDistanceTask,     // Task function
    "ReadDistance",       // Task name
    4096,                 // Stack size (bytes) - increased from 2048
    NULL,                 // Task parameters
    1,                    // Priority (1 is low)
    NULL                  // Task handle
  );
 
  xTaskCreate(
    controlOutputsTask,   // Task function to control both LEDs and LCD
    "ControlOutputs",     // Task name
    8192,                 // Stack size (bytes) - increased from 2048
    NULL,                 // Task parameters
    1,                    // Priority
    NULL                  // Task handle
  );
 
  Serial.println("ESP32 HC-SR04 Distance Sensor with FreeRTOS Started");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loc: ");
  lcd.print(location);
  lcd.setCursor(0, 1);
  lcd.print(weather);
  lcd.print(" ");
  lcd.print(temperature);
}

void loop() {
  // Empty loop as tasks are handling everything
  vTaskDelay(1000 / portTICK_PERIOD_MS); // Just keep the loop alive
}
