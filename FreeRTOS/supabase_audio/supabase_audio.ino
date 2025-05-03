#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <vector>
#include <Audio.h>
#include "SD.h"
#include "FS.h"
#include <ESP32Ping.h>   // For network diagnostics
#include <WiFiClientSecure.h> // For HTTPS

// ----------- Definitions ------------
#define LED_ONE 32
#define LED_TWO 15
#define LED_THREE 33
#define BTTN_AI 4

// microSD Card Reader connections
#define SD_CS 5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18

// I2S Connections (MAX98357)
#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC 26

// Supabase
#define supabaseUrl "https://jursmglsfqaqrxvirtiw.supabase.co"
#define supabaseKey "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imp1cnNtZ2xzZnFhcXJ4dmlydGl3Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDQ3ODkxOTEsImV4cCI6MjA2MDM2NTE5MX0.ajGbf9fLrYAA0KXzYhGFCTju-d4h-iTYTeU5WfITj3k"
#define tableName "resident_number"

const char* ssid = "TK-gacura";
const char* password = "gisaniel924";

// Audio object
Audio audio;

// ---------- Helper Functions ----------
void reconnectWiFi() {
  Serial.println("Reconnecting to WiFi...");
  
  // Disconnect from any existing WiFi connection
  WiFi.disconnect(true);
  delay(1000);
  
  // Set WiFi mode to station
  WiFi.mode(WIFI_STA);
  delay(500);
  
  // Output debug info
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Connecting to SSID: ");
  Serial.println(ssid);
  
  // Begin connection attempt
  WiFi.begin(ssid, password);
  
  // Wait for connection with timeout
  int attempts = 0;
  const int maxAttempts = 20;
  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  // Check final connection status
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi reconnected successfully");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Test connectivity by pinging Google DNS
    IPAddress ip(8, 8, 8, 8);
    bool reachable = Ping.ping(ip, 3);
    if (reachable) {
      Serial.println("Internet connection verified");
    } else {
      Serial.println("Warning: WiFi connected but internet may not be accessible");
    }
  } else {
    Serial.println("\nFailed to reconnect to WiFi");
    Serial.print("WiFi status code: ");
    Serial.println(WiFi.status());
    
    // Print helpful error information
    switch (WiFi.status()) {
      case WL_NO_SSID_AVAIL:
        Serial.println("SSID not available. Check if the router is powered on.");
        break;
      case WL_CONNECT_FAILED:
        Serial.println("Connection failed. Check the password.");
        break;
      case WL_IDLE_STATUS:
        Serial.println("WiFi is in idle state, still trying to connect.");
        break;
      default:
        Serial.println("Unknown connection issue.");
        break;
    }
  }
}

// ---------- Get Phone Numbers from Supabase ----------
void getNumbers() {
  // Verify WiFi connection first
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Attempting to reconnect...");
    reconnectWiFi();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Failed to reconnect WiFi. Cannot fetch phone numbers.");
      return;
    }
  }
  
  // Clear the existing phone numbers array
  registeredPhoneNumbers.clear();
  
  HTTPClient http;
  http.setTimeout(10000); // 10 second timeout
  
  String endpoint = String(supabaseUrl) + "/rest/v1/" + tableName;
  Serial.print("Connecting to Supabase endpoint: ");
  Serial.println(endpoint);
  
  bool connected = false;
  int attempts = 0;
  int maxAttempts = 3;
  int httpResponseCode = -1;
  
  while (!connected && attempts < maxAttempts) {
    attempts++;
    Serial.print("Supabase connection attempt ");
    Serial.print(attempts);
    Serial.print(" of ");
    Serial.println(maxAttempts);
    
    http.begin(endpoint);
    http.addHeader("apikey", supabaseKey);
    http.addHeader("Authorization", "Bearer " + String(supabaseKey));
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");
    
    // Debug network information
    Serial.print("WiFi status: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      connected = true;
    } else {
      Serial.print("Connection attempt failed: ");
      Serial.println(http.errorToString(httpResponseCode));
      delay(1000); // Wait before retry
    }
  }
  
  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("Supabase response received successfully");
    
    // Parse JSON response
    DynamicJsonDocument doc(4096); // Increased buffer size
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
        if (entry.containsKey("id") && entry.containsKey("number")) {
          int id = entry["id"];
          String number = entry["number"].as<String>();
          
          // Add number to our array
          registeredPhoneNumbers.push_back(number);
          
          // Add ID to our known IDs list if not already there
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
            
            // Play confirmation sound
            if (SD.exists("NEW-NUM-REG-HIGH.mp3")) {
              Serial.println("Playing registration sound...");
              audio.connecttoFS(SD, "NEW-NUM-REG-HIGH.mp3");
              while (audio.isRunning()) {
                audio.loop();
              }
            } else {
              Serial.println("Audio file not found on SD card");
            }
            
            knownIds.push_back(id);
            Serial.print("New Number Added: ");
            Serial.println(number);
            delay(1500);
            
            digitalWrite(AI_LED_ONE, LOW);
            digitalWrite(AI_LED_TWO, LOW);
            digitalWrite(AI_LED_THREE, LOW);
          }
        } else {
          Serial.println("Warning: Invalid entry format in Supabase response");
        }
      }
      
      Serial.print("Total registered numbers: ");
      Serial.println(registeredPhoneNumbers.size());
    }
  } else {
    Serial.print("Error getting entries. HTTP Response code: ");
    Serial.println(httpResponseCode);
    if (httpResponseCode == -1) {
      Serial.println("Possible causes: Timeout, DNS failure, or connection error");
    }
  }
  
  http.end();
}

void aiButtonTask(void *parameter) {
  pinMode(BTTN_AI, INPUT_PULLUP);

  while (true) {
    if (digitalRead(BTTN_AI) == LOW) {
      Serial.println("Button pressed: Generating AI suggestion...");
      getAISuggestion();
      vTaskDelay(2000 / portTICK_PERIOD_MS);  // debounce
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// Task to periodically check for new phone numbers
void phoneNumbersTask(void *parameter) {
  while (true) {
    getNumbers();
    vTaskDelay(2000 / portTICK_PERIOD_MS);  // Check every 30 seconds
  }
}


// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(1000); // Short delay to stabilize
  Serial.println("\n\n=== PRAF Flood Alert System Starting ===");

  // Pin configuration
  pinMode(LED_ONE, OUTPUT);
  pinMode(LED_TWO, OUTPUT);
  pinMode(LED_THREE, OUTPUT);
  
  pinMode(AI_LED_ONE, OUTPUT);
  pinMode(AI_LED_TWO, OUTPUT);
  pinMode(AI_LED_THREE, OUTPUT);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Test LEDs
  Serial.println("Testing LEDs...");
  digitalWrite(LED_ONE, HIGH);
  digitalWrite(LED_TWO, HIGH);
  digitalWrite(LED_THREE, HIGH);
  delay(500);
  digitalWrite(LED_ONE, LOW);
  digitalWrite(LED_TWO, LOW);
  digitalWrite(LED_THREE, LOW);
  
  // Initialize SD card for audio files
  Serial.println("Initializing SD card...");
  if (!SD.begin()) {
    Serial.println("SD Card initialization failed. Audio alerts will not work.");
  } else {
    Serial.println("SD Card initialized successfully.");
    // List files on SD card (for debugging)
    File root = SD.open("/");
    // printDirectory(root, 0);
    root.close();
  }

  // Audio setup with I2S DAC
  Serial.println("Setting up Audio...");
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(21); // 0...21

  // LCD Init
  Serial.println("Initializing LCD...");
  Wire.begin(); // Initialize I2C
  
  // Check if LCD is responding
  Wire.beginTransmission(0x27);
  bool lcdFound = (Wire.endTransmission() == 0);
  
  if (lcdFound) {
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("PRAF Flood");
    lcd.setCursor(0, 1);
    lcd.print("Alert System");
    Serial.println("LCD initialized successfully.");
  } else {
    Serial.println("LCD not found! Check connections.");
  }

  // Wi-Fi Connection with timeout
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA); // Set WiFi to station mode
  WiFi.begin(ssid, password);
  
  int wifiTimeout = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTimeout < 20) {
    delay(500);
    Serial.print(".");
    wifiTimeout++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed! Will retry in background.");
  }
  
  // Set default AI suggestion in case API call fails
  AISuggestion = "PRAF Technology Weather Update: May posibilidad ng pagbaha sa Quezon City dahil sa malakas na ulan. Manatiling alerto at maghanda ng emergency kit.";

  // Initial fetch of phone numbers (only if WiFi is connected)
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Performing initial fetch of phone numbers...");
    getNumbers();
  }

  // Create tasks with proper stack sizes
  Serial.println("Starting system tasks...");
  xTaskCreatePinnedToCore(aiButtonTask, "AI Button", 8192, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(phoneNumbersTask, "Phone Numbers", 4096, NULL, 1, NULL, 1);
  
  Serial.println("System initialization complete!");
}

// Function to print directory contents (for debugging SD card)
void printDirectory(File dir, int numTabs) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void loop() {
  // Empty loop, tasks handle everything
}
