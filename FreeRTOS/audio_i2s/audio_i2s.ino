/*
 * ESP32 Button MP3 Player with WiFi and Supabase integration
 * When buttons are pressed, performs different actions:
 * - Main button plays audio from SD card via MAX98357 I2S DAC
 * - SMS button fetches phone numbers from Supabase
 * Using FreeRTOS for task management
 */

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <SD.h>
#include <SPI.h>
#include "Audio.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <vector>

// Pin Definitions
#define BUTTON_PIN 4      // Main button pin
#define BTTN_SMS 2        // SMS button pin
#define DEBOUNCE_TIME 50  // Debounce time in milliseconds

// LED Indicators
#define AI_LED_ONE 14     // LED indicators for status
#define AI_LED_TWO 12
#define AI_LED_THREE 13

// Wifi Configuration
#define SSID "TK-gacura"
#define PASSWORD "gisaniel924"

// Supabase Configuration
#define supabaseUrl "https://jursmglsfqaqrxvirtiw.supabase.co"
#define supabaseKey "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imp1cnNtZ2xzZnFhcXJ4dmlydGl3Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDQ3ODkxOTEsImV4cCI6MjA2MDM2NTE5MX0.ajGbf9fLrYAA0KXzYhGFCTju-d4h-iTYTeU5WfITj3k"
#define tableName "resident_number"

// microSD Card Reader connections
#define SD_CS 5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18

// I2S Connections (MAX98357)
#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC 26

// Audio object for ESP32-audioI2S library
Audio audio;

// Queue handles for communication between tasks
QueueHandle_t buttonQueue;
QueueHandle_t smsButtonQueue;

// Variable to track button states
volatile bool buttonPressed = false;
volatile bool smsButtonPressed = false;
volatile uint32_t lastDebounceTime = 0;
volatile uint32_t lastSmsDebounceTime = 0;

// Flag to indicate if audio is currently playing
volatile bool isAudioPlaying = false;

// Storage for phone numbers and IDs
std::vector<String> registeredPhoneNumbers;
std::vector<int> knownIds;

// Task handles
TaskHandle_t buttonTaskHandle = NULL;
TaskHandle_t audioTaskHandle = NULL;
TaskHandle_t smsButtonTaskHandle = NULL;
TaskHandle_t supabaseTaskHandle = NULL;

// Main button monitoring task
void buttonTask(void *parameter) {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  bool buttonState;
  bool lastButtonState = HIGH;
  
  while(1) {
    buttonState = digitalRead(BUTTON_PIN);
    
    // Check if button state changed
    if (buttonState != lastButtonState) {
      // Debounce
      if ((millis() - lastDebounceTime) > DEBOUNCE_TIME) {
        lastDebounceTime = millis();
        
        // If button is pressed (LOW due to pull-up) AND audio is not currently playing
        if (buttonState == LOW && !isAudioPlaying) {
          int pressEvent = 1;
          xQueueSend(buttonQueue, &pressEvent, 0);
        }
        
        lastButtonState = buttonState;
      }
    }
    
    vTaskDelay(10 / portTICK_PERIOD_MS);  // Small delay to prevent CPU hogging
  }
}

// SMS button monitoring task
void smsButtonTask(void *parameter) {
  pinMode(BTTN_SMS, INPUT_PULLUP);
  bool buttonState;
  bool lastButtonState = HIGH;
  
  while(1) {
    buttonState = digitalRead(BTTN_SMS);
    
    // Check if button state changed
    if (buttonState != lastButtonState) {
      // Debounce
      if ((millis() - lastSmsDebounceTime) > DEBOUNCE_TIME) {
        lastSmsDebounceTime = millis();
        
        // If button is pressed (LOW due to pull-up)
        if (buttonState == LOW) {
          int pressEvent = 1;
          xQueueSend(smsButtonQueue, &pressEvent, 0);
        }
        
        lastButtonState = buttonState;
      }
    }
    
    vTaskDelay(10 / portTICK_PERIOD_MS);  // Small delay to prevent CPU hogging
  }
}

// Audio task - handles playing audio
void audioTask(void *parameter) {
  int event;
  
  while(1) {
    // Wait for event from button task
    if (xQueueReceive(buttonQueue, &event, portMAX_DELAY)) {
      if (event == 1) {
        Serial.println("Button pressed! Playing audio file...");
        
        // Set flag to indicate audio is playing
        isAudioPlaying = true;
        
        // Stop any currently playing audio (shouldn't happen with our protection, but just in case)
        audio.stopSong();
        
        // Play MP3 file from SD card
        audio.connecttoFS(SD, "/DEVICE-START-VOICE.mp3");
        
        // Wait until audio is finished
        while (audio.isRunning()) {
          audio.loop();
          vTaskDelay(1 / portTICK_PERIOD_MS);
        }
        
        Serial.println("Audio playback complete");
        
        // Clear flag to allow new playback
        isAudioPlaying = false;
      }
    }
    
    // No delay needed as task blocks on queue
  }
}

// Supabase task - handles SMS button presses and API interactions
void supabaseTask(void *parameter) {
  int event;
  
  while(1) {
    // Wait for event from SMS button task
    if (xQueueReceive(smsButtonQueue, &event, portMAX_DELAY)) {
      if (event == 1) {
        Serial.println("SMS Button pressed! Fetching phone numbers...");
        
        // Only proceed if not currently playing audio
        if (!isAudioPlaying) {
          // Call function to get numbers from Supabase
          getNumbers();
        } else {
          Serial.println("Audio is playing, ignoring SMS button press");
        }
      }
    }
    
    // No delay needed as task blocks on queue
  }
}

// Initialize WiFi connection
void connectWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(SSID, PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("WiFi connection failed");
  }
}

// Function to get registered phone numbers from Supabase
void getNumbers() {
  // Check WiFi status first and make a serious attempt to reconnect
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, attempting to reconnect...");
    WiFi.disconnect();
    delay(1000);
    
    // Multiple reconnection attempts
    for (int i = 0; i < 3; i++) {
      WiFi.begin(SSID, PASSWORD);
      Serial.print("WiFi reconnection attempt ");
      Serial.print(i + 1);
      Serial.println("...");
      
      // Wait for connection with longer timeout
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi reconnected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        break;
      } else {
        Serial.println("\nWiFi reconnection failed, trying again...");
        WiFi.disconnect();
        delay(1000);
      }
    }
    
    // If still not connected, abort operation
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi connection failed after multiple attempts. Cannot fetch data.");
      return;
    }
  }
  
  // Clear the existing phone numbers array
  registeredPhoneNumbers.clear();
  
  // Create a secure WiFi client
  WiFiClientSecure *client = new WiFiClientSecure;
  if(client) {
    // Skip SSL certificate validation (for testing only)
    client->setInsecure();
    
    HTTPClient https;
    // Increase timeouts for more reliable connection
    https.setTimeout(10000); // 10 second timeout
    
    // Specify HTTPS and full URL
    String endpoint = String(supabaseUrl) + "/rest/v1/" + tableName;
    Serial.print("Connecting to endpoint: ");
    Serial.println(endpoint);
    
    // Begin with proper HTTPS configuration using secure client
    bool httpBeginSuccess = https.begin(*client, endpoint);
    if (!httpBeginSuccess) {
      Serial.println("Failed to connect to the server. Check URL format.");
      delete client;
      return;
    }
    
    // Add all required headers
    https.addHeader("apikey", supabaseKey);
    https.addHeader("Authorization", "Bearer " + String(supabaseKey));
    https.addHeader("Content-Type", "application/json");
    https.addHeader("Prefer", "return=representation"); // Get full representation
    
    Serial.println("Sending GET request...");
    int httpResponseCode = https.GET();
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    
    if (httpResponseCode == 200) {
      String response = https.getString();
      Serial.println("Response received successfully");
      
      // Parse JSON response
      DynamicJsonDocument doc(4096); // Increased buffer size for larger responses
      DeserializationError error = deserializeJson(doc, response);
      if (error) {
        Serial.print("JSON deserialization failed: ");
        Serial.println(error.c_str());
      } else {
        JsonArray array = doc.as<JsonArray>();
        Serial.print("Found ");
        Serial.print(array.size());
        Serial.println(" phone numbers.");
        
        for (JsonVariant entry : array) {
          int id = entry["id"];
          String number = entry["number"].as<String>();
          
          // Add number to our array
          registeredPhoneNumbers.push_back(number);
          
          // Check if this ID is new
          bool isNewId = true;
          for (int knownId : knownIds) {
            if (id == knownId) {
              isNewId = false;
              break;
            }
          }
          
          if (isNewId) {
            digitalWrite(AI_LED_ONE, HIGH);
            digitalWrite(AI_LED_TWO, HIGH);
            digitalWrite(AI_LED_THREE, HIGH);
            
            // Wait for any current audio to finish
            while (isAudioPlaying) {
              vTaskDelay(100 / portTICK_PERIOD_MS);
            }
            
            // Set playing flag to prevent other audio playback
            isAudioPlaying = true;
            
            // Play confirmation sound
            audio.connecttoFS(SD, "/NEW-NUM-REG-HIGH.mp3");
            while (audio.isRunning()) {
              audio.loop();
            }
            
            knownIds.push_back(id);
            Serial.print("New Number Added: ");
            Serial.println(number);
            
            delay(1500);
            digitalWrite(AI_LED_ONE, LOW);
            digitalWrite(AI_LED_TWO, LOW);
            digitalWrite(AI_LED_THREE, LOW);
            
            // Clear playing flag
            isAudioPlaying = false;
          }
        }
        
        Serial.print("Total registered numbers: ");
        Serial.println(registeredPhoneNumbers.size());
      }
    } else {
      // Better error handling
      if (httpResponseCode == -1) {
        Serial.println("Error: Connection failed or timed out");
        Serial.println("Make sure WiFi is stable and the server is accessible");
      } else if (httpResponseCode == -2) {
        Serial.println("Error: Unable to connect to the server (DNS lookup failed)");
      } else if (httpResponseCode == -3) {
        Serial.println("Error: Writing to the server failed");
      } else if (httpResponseCode == -4) {
        Serial.println("Error: Connection timed out");
      } else if (httpResponseCode == -5) {
        Serial.println("Error: No server response received");
      } else if (httpResponseCode == -6) {
        Serial.println("Error: Unable to create SSL/TLS connection");
      } else if (httpResponseCode == -7) {
        Serial.println("Error: Connection to server closed prematurely");
      } else if (httpResponseCode >= 400) {
        Serial.print("Server error: HTTP Response code: ");
        Serial.println(httpResponseCode);
      }
      
      // Flash error LEDs
      for (int i = 0; i < 3; i++) {
        digitalWrite(AI_LED_ONE, HIGH);
        delay(200);
        digitalWrite(AI_LED_ONE, LOW);
        delay(200);
      }
    }
    
    https.end();
    Serial.println("HTTPS connection closed");
    delete client;
  } else {
    Serial.println("Unable to create secure client");
  }
}

// Initialize SD card
bool initSDCard() {
  // Configure SPI pins for SD card
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  
  if (!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    return false;
  }
  
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return false;
  }

  Serial.println("SD Card Initialized");
  
  // Check if our audio files exist
  if (!SD.exists("/DEVICE-START-VOICE.mp3")) {
    Serial.println("Cannot find DEVICE-START-VOICE.mp3 file");
    return false;
  }
  
  if (!SD.exists("/NEW-NUM-REG-HIGH.mp3")) {
    Serial.println("Cannot find NEW-NUM-REG-HIGH.mp3 file");
    return false;
  }
  
  Serial.println("Found MP3 files");
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Button Audio Player with Supabase Integration");
  
  // Initialize LED pins
  pinMode(AI_LED_ONE, OUTPUT);
  pinMode(AI_LED_TWO, OUTPUT);
  pinMode(AI_LED_THREE, OUTPUT);
  
  // Turn off all LEDs initially
  digitalWrite(AI_LED_ONE, LOW);
  digitalWrite(AI_LED_TWO, LOW);
  digitalWrite(AI_LED_THREE, LOW);
  
  // Initialize SD card
  if (!initSDCard()) {
    Serial.println("SD card initialization failed!");
    while(1); // Stop execution if SD card initialization fails
  }
  
  // Connect to WiFi
  connectWiFi();
  
  // Set up I2S configuration for MAX98357
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(100); // 0-21
  
  // Create queues for button events
  buttonQueue = xQueueCreate(10, sizeof(int));
  smsButtonQueue = xQueueCreate(10, sizeof(int));
  
  if (buttonQueue == NULL || smsButtonQueue == NULL) {
    Serial.println("Error creating the queues");
    return;
  }
  
  // Create tasks
  xTaskCreate(
    buttonTask,          // Task function
    "ButtonTask",        // Name of task
    2048,                // Stack size
    NULL,                // Parameter to pass
    1,                   // Priority
    &buttonTaskHandle    // Task handle
  );
  
  xTaskCreate(
    smsButtonTask,       // Task function
    "SMSButtonTask",     // Name of task
    2048,                // Stack size
    NULL,                // Parameter to pass
    1,                   // Priority
    &smsButtonTaskHandle // Task handle
  );
  
  xTaskCreate(
    audioTask,           // Task function
    "AudioTask",         // Name of task
    4096,                // Stack size (increased for audio processing)
    NULL,                // Parameter to pass
    1,                   // Priority
    &audioTaskHandle     // Task handle
  );
  
  xTaskCreate(
    supabaseTask,        // Task function
    "SupabaseTask",      // Name of task
    16384,               // Stack size (increased significantly for HTTPS and JSON)
    NULL,                // Parameter to pass
    1,                   // Priority
    &supabaseTaskHandle  // Task handle
  );
  
  // Flash LEDs to indicate successful initialization
  for (int i = 0; i < 3; i++) {
    digitalWrite(AI_LED_ONE, HIGH);
    digitalWrite(AI_LED_TWO, HIGH);
    digitalWrite(AI_LED_THREE, HIGH);
    delay(200);
    digitalWrite(AI_LED_ONE, LOW);
    digitalWrite(AI_LED_TWO, LOW);
    digitalWrite(AI_LED_THREE, LOW);
    delay(200);
  }
}

void loop() {
  // Empty as FreeRTOS tasks handle everything
  
  // Check WiFi status periodically and reconnect if needed
  static unsigned long lastWifiCheck = 0;
  if (millis() - lastWifiCheck > 30000) { // Check every 30 seconds
    lastWifiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected in main loop, attempting to reconnect...");
      // More aggressive reconnection
      WiFi.disconnect();
      delay(1000);
      WiFi.begin(SSID, PASSWORD);
      
      // Wait for connection with timeout
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi reconnected in main loop");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
      } else {
        Serial.println("\nWiFi reconnection failed in main loop");
      }
    } else {
      // Even if connected, print info periodically
      Serial.println("WiFi still connected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    }
  }
  
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
