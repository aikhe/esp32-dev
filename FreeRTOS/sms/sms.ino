#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>

// ----------- Definitions ------------
#define LED_ONE 13
#define LED_TWO 12
#define LED_THREE 14
#define BTTN_SMS 4

// Supabase
#define supabaseUrl "https://jursmglsfqaqrxvirtiw.supabase.co"
#define supabaseKey "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imp1cnNtZ2xzZnFhcXJ4dmlydGl3Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDQ3ODkxOTEsImV4cCI6MjA2MDM2NTE5MX0.ajGbf9fLrYAA0KXzYhGFCTju-d4h-iTYTeU5WfITj3k"
#define tableName "resident_number"

const char* ssid = "TK-gacura";
const char* password = "gisaniel924";

// Phone numbers and known IDs
std::vector<String> registeredPhoneNumbers;
std::vector<int> knownIds;

// ---------- Helper Functions ----------
void reconnectWiFi() {
  Serial.println("Reconnecting to WiFi...");
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi reconnected");
  } else {
    Serial.println("\nFailed to reconnect to WiFi");
  }
}

// ---------- Get Phone Numbers from Supabase ----------
void getNumbers() {
  if (WiFi.status() != WL_CONNECTED) {
    reconnectWiFi();
  }
  // Clear the existing phone numbers array
  registeredPhoneNumbers.clear();
  HTTPClient http;
  String endpoint = String(supabaseUrl) + "/rest/v1/" + tableName;
  http.begin(endpoint);
  http.addHeader("apikey", supabaseKey);
  http.addHeader("Authorization", "Bearer " + String(supabaseKey));
  http.addHeader("Content-Type", "application/json");
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
          digitalWrite(LED_ONE, HIGH);
          digitalWrite(LED_TWO, HIGH);
          digitalWrite(LED_THREE, HIGH);
          
          knownIds.push_back(id);
          Serial.print("New Number Added: ");
          Serial.println(number);
          delay(1500);
          digitalWrite(LED_ONE, LOW);
          digitalWrite(LED_TWO, LOW);
          digitalWrite(LED_THREE, LOW);
        }
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

// Task to handle button press for getting numbers
void buttonTask(void *parameter) {
  pinMode(BTTN_SMS, INPUT_PULLUP);  // Set button pin as input with internal pull-up
  
  while (true) {
    // Read the current button state
    bool reading = digitalRead(BTTN_SMS);
    
    // Check if the reading has changed (for debouncing)
    if (reading != lastButtonState) {
      lastDebounceTime = millis();
    }
    
    // If enough time has passed since the last change, process the button press
    if ((millis() - lastDebounceTime) > debounceDelay) {
      // If the button state has changed and is now pressed (LOW)
      if (reading == LOW && lastButtonState == HIGH) {
        Serial.println("Button pressed: Fetching phone numbers...");
        getNumbers();
      }
    }
    
    // Update last button state
    lastButtonState = reading;
    
    vTaskDelay(10 / portTICK_PERIOD_MS);  // Small delay for task scheduling
  }
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);

  pinMode(LED_ONE, OUTPUT);
  pinMode(LED_TWO, OUTPUT);
  pinMode(LED_THREE, OUTPUT);
  pinMode(BTTN_SMS, INPUT_PULLUP);  // Set the button pin as input with pull-up

  // Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");

  Serial.println("System ready. Press the button to fetch phone numbers.");

  // Create task for button handling
  xTaskCreatePinnedToCore(buttonTask, "Button Handler", 4096, NULL, 1, NULL, 1);
}

void loop() {
  // Empty loop, tasks handle everything
}
