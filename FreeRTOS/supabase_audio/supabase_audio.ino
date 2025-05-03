#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include "Audio.h"
#include <vector>
#include <WiFiClientSecure.h> // For HTTPS

// ----------- Definitions ------------
#define BTTN_GET_NUMBERS 2
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

#define DEBOUNCE_TIME 50  // Debounce time in milliseconds
volatile uint32_t lastSmsDebounceTime = 0;

// Supabase
#define supabaseUrl "https://jursmglsfqaqrxvirtiw.supabase.co"
#define supabaseKey "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imp1cnNtZ2xzZnFhcXJ4dmlydGl3Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDQ3ODkxOTEsImV4cCI6MjA2MDM2NTE5MX0.ajGbf9fLrYAA0KXzYhGFCTju-d4h-iTYTeU5WfITj3k"
#define tableName "resident_number"

// Predefined weather info
String location = "Quezon City";
String weatherDescription = "maulan at may malalakas na hangin";
float temperature = 27.5;
float feelsLike = 29.0;
int humidity = 87;

String AISuggestion = "";

const char *ssid = "TK-gacura";
const char *password = "gisaniel924";

const char* geminiApiKey = "AIzaSyD_g_WAsPqPKxltdOJt8VZw4uu359D3XXA";

// Phone numbers and known IDs
std::vector<String> registeredPhoneNumbers;
std::vector<int> knownIds;

// Task handles
TaskHandle_t buttonTaskHandle = NULL;

// Create Audio object
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
  } else {
    Serial.println("\nFailed to reconnect to WiFi");
    Serial.print("WiFi status code: ");
    Serial.println(WiFi.status());
  }
}

// ---------- Get Phone Numbers from Supabase ----------
void getNumbers() {
  if (WiFi.status() != WL_CONNECTED) {
    reconnectWiFi();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Failed to connect to WiFi. Cannot get numbers.");
      return;
    }
  }

  // Clear the existing phone numbers array
  registeredPhoneNumbers.clear();

  HTTPClient http;
  String endpoint = String(supabaseUrl) + "/rest/v1/" + tableName;

  http.begin(endpoint);
  http.addHeader("apikey", supabaseKey);
  http.addHeader("Authorization", "Bearer " + String(supabaseKey));
  http.addHeader("Content-Type", "application/json");

  Serial.println("Fetching numbers from Supabase...");
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    String response = http.getString();

    // Parse JSON response
    DynamicJsonDocument doc(2048);
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

        // Add ID to our known IDs list if not already there
        bool isNewId = true;
        for (int knownId : knownIds) {
          if (id == knownId) {
            isNewId = false;
            break;
          }
        }

        if (isNewId) {
          knownIds.push_back(id);
          Serial.print("New Number Added: ");
          Serial.println(number);

          // Play confirmation sound
          audio.connecttoFS(SD, "NEW-NUM-REG-HIGH.mp3");
          while (audio.isRunning()) {
            audio.loop();
            vTaskDelay(1 / portTICK_PERIOD_MS);
          }
        }

        // Print all phone numbers
        Serial.print("ID: ");
        Serial.print(id);
        Serial.print(" | Number: ");
        Serial.println(number);
      }

      Serial.print("Total registered numbers: ");
      Serial.println(registeredPhoneNumbers.size());
    }
  } else {
    Serial.print("Error getting entries. HTTP Response code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

// ---------- Button Task Function ----------
void smsButtonTask(void *parameter) {
  pinMode(BTTN_GET_NUMBERS, INPUT_PULLUP);
  bool buttonState;
  bool lastButtonState = HIGH;

  while (1) {
    buttonState = digitalRead(BTTN_GET_NUMBERS);

    // Check if button state changed
    if (buttonState != lastButtonState) {
      // Debounce
      if ((millis() - lastSmsDebounceTime) > DEBOUNCE_TIME) {
        lastSmsDebounceTime = millis();

        // If button is pressed (LOW due to pull-up)
        if (buttonState == LOW) {
          getNumbers();
        }

        lastButtonState = buttonState;
      }
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);  // Small delay to prevent CPU hogging
  }
}

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

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(1000);  // Short delay to stabilize
  Serial.println("\n\n=== PRAF Supabase Number Fetcher (FreeRTOS) ===");

  // Wi-Fi Connection
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);  // Set WiFi to station mode
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
    Serial.println("\nWiFi connection failed! Will retry when button is pressed.");
  }

  // Set microSD Card CS as OUTPUT and set HIGH
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  // Initialize SPI bus for microSD Card
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  // Initialize microSD card with custom SPI
  if (!SD.begin(SD_CS, SPI)) {
    Serial.println("Error accessing microSD card!");
    while (true)
      ;
  }

  Serial.println("microSD card initialized.");

  // Audio setup with I2S DAC
  Serial.println("Setting up Audio...");
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(21);  // 0...21

  // Create FreeRTOS tasks
  xTaskCreatePinnedToCore(aiButtonTask, "AI Button", 8192, NULL, 1, NULL, 1);
  xTaskCreate(
    smsButtonTask,     // Task function
    "ButtonTask",      // Task name
    8096,              // Stack size in words (more for JSON processing)
    NULL,              // Task parameters
    1,                 // Priority (1 is low, configMAX_PRIORITIES-1 is high)
    &buttonTaskHandle  // Task handle
  );
  xTaskCreatePinnedToCore(scrollLCDTask, "LCD Scroll", 2048, NULL, 1, NULL, 1);

  Serial.println("System ready! Press the button to fetch numbers from Supabase.");
}

void loop() {
  // Nothing to do here, FreeRTOS tasks handle everything
  vTaskDelay(portMAX_DELAY);  // Just sleep
}
