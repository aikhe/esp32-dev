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

// HC-SRO4 Sensor
#define TRIG_PIN 17
#define ECHO_PIN 16

// AI LEDs (assuming they're different from the regular LEDs)
#define AI_LED_ONE 13
#define AI_LED_TWO 12
#define AI_LED_THREE 14

// Supabase
#define supabaseUrl "https://jursmglsfqaqrxvirtiw.supabase.co"
#define supabaseKey "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imp1cnNtZ2xzZnFhcXJ4dmlydGl3Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDQ3ODkxOTEsImV4cCI6MjA2MDM2NTE5MX0.ajGbf9fLrYAA0KXzYhGFCTju-d4h-iTYTeU5WfITj3k"
#define tableName "resident_number"

const char* ssid = "TK-gacura";
const char* password = "gisaniel924";
const char* geminiApiKey = "AIzaSyD_g_WAsPqPKxltdOJt8VZw4uu359D3XXA";

// Predefined weather info
String location = "Quezon City";
String weatherDescription = "maulan at may malalakas na hangin";
float temperature = 27.5;
float feelsLike = 29.0;
int humidity = 87;

String AISuggestion = "";

// Audio object
Audio audio;

// Phone numbers and known IDs
std::vector<String> registeredPhoneNumbers;
std::vector<int> knownIds;

// LCD I2C init (16x2)
LiquidCrystal_I2C lcd(0x27, 16, 2);

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

// ---------- AI Suggestion Function ----------
void getAISuggestion() {
  // Check WiFi connection and attempt to reconnect if needed
  if (WiFi.status() != WL_CONNECTED) {
    reconnectWiFi();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("No WiFi connection. Using default suggestion.");
      AISuggestion = "PRAF Technology Weather Update: May posibilidad ng pagbaha sa Quezon City dahil sa malakas na ulan. Manatiling alerto at maghanda ng emergency kit.";
      return;
    }
  }

  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate verification
  
  HTTPClient http;
  http.setTimeout(10000); // 10 second timeout
  
  Serial.println("Preparing Gemini API request...");
  
  String prompt = "Provide a short and helpful suggestion to inform residents about the current weather and keep them safe.\n\n";
  prompt += "- Weather Details:\n";
  prompt += "  - City: " + location + "\n";
  prompt += "  - Weather: " + weatherDescription + "\n";
  prompt += "  - Temperature: " + String(temperature, 2) + "°C\n";
  prompt += "  - Feels like: " + String(feelsLike, 2) + "°C\n";
  prompt += "  - Humidity: " + String(humidity, 2) + "%\n\n";
  prompt += "Instructions:\n";
  prompt += "- Write the message like a weather forecast-casual, clear, and understandable for most people.\n";
  prompt += "- Start with: \"PRAF Technology Weather Update:\".\n";
  prompt += "- Next sentence should note the location/city:\".\n";
  prompt += "- The message should be one sentence long and include a note that it's from PRAF Technology.\n";
  prompt += "- If the weather poses a flood risk, alert the residents.\n";
  prompt += "- If flooding is unlikely, suggest a safe way to deal with the weather while reassuring them.\n";
  prompt += "- Maintain a formal tone and avoid AI-like phrasing.\n";
  prompt += "- Do not use uncertain words like \"naman.\"\n";
  prompt += "- And most importantly mainly use tagalog.\n";
  prompt += "- Structure:\n";
  prompt += "  1. Start with the flood update.\n";
  prompt += "  2. Then, provide the weather update.\n";
  prompt += "  3. End with a safety tip.\n";
  prompt += "- Do not include greetings-just start with the message.";

  StaticJsonDocument<2048> requestDoc;
  JsonArray contents = requestDoc.createNestedArray("contents");
  JsonObject content = contents.createNestedObject();
  JsonArray parts = content.createNestedArray("parts");
  JsonObject part = parts.createNestedObject();
  part["text"] = prompt;

  JsonObject generationConfig = requestDoc.createNestedObject("generationConfig");
  generationConfig["temperature"] = 0.7;
  generationConfig["topP"] = 0.9;
  generationConfig["maxOutputTokens"] = 200;

  String requestBody;
  serializeJson(requestDoc, requestBody);

  String geminiUrl = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=" + String(geminiApiKey);

  // Debug WiFi connection
  Serial.print("WiFi status before API call: ");
  Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Try to establish connection with retries
  bool connected = false;
  int tries = 0;
  int maxTries = 3;
  int httpCode = -1;
  
  while (!connected && tries < maxTries) {
    tries++;
    Serial.print("API connection attempt ");
    Serial.print(tries);
    Serial.print(" of ");
    Serial.println(maxTries);
    
    if (http.begin(client, geminiUrl)) {
      http.addHeader("Content-Type", "application/json");
      httpCode = http.POST(requestBody);
      
      if (httpCode > 0) {
        connected = true;
      } else {
        Serial.print("Connection failed, error: ");
        Serial.println(http.errorToString(httpCode));
        delay(1000); // Wait before retry
      }
    } else {
      Serial.println("Failed to begin HTTP connection");
      delay(1000);
    }
  }

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Gemini API Response: " + payload);

    StaticJsonDocument<2048> responseDoc;
    DeserializationError error = deserializeJson(responseDoc, payload);

    if (!error && responseDoc.containsKey("candidates") && 
        responseDoc["candidates"][0]["content"]["parts"][0].containsKey("text")) {
      AISuggestion = responseDoc["candidates"][0]["content"]["parts"][0]["text"].as<String>();
      Serial.println("\n==== AI WEATHER SUGGESTION ====");
      Serial.println(AISuggestion);
      Serial.println("===============================\n");
    } else {
      Serial.println("Error parsing Gemini API response");
      AISuggestion = "PRAF Technology Weather Update: May posibilidad ng pagbaha sa Quezon City dahil sa malakas na ulan. Manatiling alerto at maghanda ng emergency kit.";
    }
  } else {
    Serial.print("Failed to connect to Gemini API, HTTP code: ");
    Serial.println(httpCode);
    Serial.println("Using default suggestion instead.");
    AISuggestion = "PRAF Technology Weather Update: May posibilidad ng pagbaha sa Quezon City dahil sa malakas na ulan. Manatiling alerto at maghanda ng emergency kit.";
  }

  http.end();
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

// ---------- Distance Measurement Function ----------
float getDistance() {
  const int numReadings = 5;  // Number of readings to average
  float readings[numReadings];
  float sum = 0;
  float validReadings = 0;
  
  // Take multiple readings
  for (int i = 0; i < numReadings; i++) {
    // Clear the trigger pin
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    
    // Set the trigger pin high for 10 microseconds
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    // Read the echo pin
    float duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
    float distance = duration * 0.034 / 2;
    
    // Validate reading (HC-SR04 typically works between 2cm and 400cm)
    if (distance >= 2 && distance <= 400) {
      readings[i] = distance;
      sum += distance;
      validReadings++;
    }
    
    delay(10); // Small delay between readings
  }
  
  // If we have valid readings, return the average
  if (validReadings > 0) {
    float average = sum / validReadings;
    
    // Additional validation - if the reading is too different from previous readings, reject it
    static float lastValidReading = 0;
    if (lastValidReading == 0) {
      lastValidReading = average;
    } else if (abs(average - lastValidReading) > 50) { // If difference is more than 50cm
      return lastValidReading; // Return last valid reading instead
    }
    
    lastValidReading = average;
    return average;
  }
  
  return -1; // Return -1 if no valid readings
}

// ---------- Tasks ----------
void waveLEDTask(void *parameter) {
  const int leds[] = {LED_ONE, LED_TWO, LED_THREE};
  const int count = sizeof(leds) / sizeof(leds[0]);

  while (true) {
    for (int i = 0; i < count; i++) {
      for (int j = 0; j < count; j++) {
        digitalWrite(leds[j], j == i ? HIGH : LOW);
      }
      vTaskDelay(200 / portTICK_PERIOD_MS);
    }
  }
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

void scrollLCDTask(void *parameter) {
  String msg = ">> PRAF Tech Alert System";
  String paddedMsg = msg + "    ";  // Added more padding for smoother transition

  // Create a circular buffer by adding end portion to the start
  paddedMsg = paddedMsg.substring(paddedMsg.length() - 16) + paddedMsg;

  int len = paddedMsg.length();
  int pos = len - 16;  // Start from the end
  const int interval = 30;  // Scroll speed
  unsigned long lastUpdate = 0;

  while (true) {
    unsigned long currentTime = millis();
    if (currentTime - lastUpdate >= interval) {
      // Move left-to-right by decreasing the position
      pos = (pos - 1 + (len - 16)) % (len - 16);
      
      // Extract current window
      String scrollSegment = paddedMsg.substring(pos, pos + 16);
      
      // Display on both rows
      lcd.setCursor(0, 0);
      lcd.print(scrollSegment);

      lcd.setCursor(0, 1);
      lcd.print(scrollSegment);

      lastUpdate = currentTime;
    }

    vTaskDelay(1);  // Yield for multitasking
  }
}

// Task to periodically check for new phone numbers
void phoneNumbersTask(void *parameter) {
  while (true) {
    getNumbers();
    vTaskDelay(2000 / portTICK_PERIOD_MS);  // Check every 30 seconds
  }
}

// Task to monitor water level using the HC-SR04 sensor
void waterLevelTask(void *parameter) {
  while (true) {
    float distance = getDistance();
    
    if (distance > 0) {
      // Serial.print("Water level distance: ");
      // Serial.print(distance);
      // Serial.println(" cm");

      // Add your water level alert logic here
      if (distance < 50) { // Example threshold
        Serial.println("WARNING: Water level is rising!");
      }
    } else {
      Serial.println("Error: Invalid water level reading");
    }
    
    vTaskDelay(5000 / portTICK_PERIOD_MS);  // Check every 5 seconds
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
  xTaskCreatePinnedToCore(waveLEDTask, "LED Wave", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(aiButtonTask, "AI Button", 8192, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(scrollLCDTask, "LCD Scroll", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(phoneNumbersTask, "Phone Numbers", 4096, NULL, 1, NULL, 1);
  // xTaskCreatePinnedToCore(waterLevelTask, "Water Level", 2048, NULL, 1, NULL, 1);
  
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

  // Play MP3 file from SD card
  audio.connecttoFS(SD, "/DEVICE-START-VOICE.mp3");
  
  // Wait until audio is finished
  while (audio.isRunning()) {
    audio.loop();
  }
