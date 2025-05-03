#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>

// ----------- Definitions ------------
#define BTTN_GET_NUMBERS 4

#define DEBOUNCE_TIME 50  // Debounce time in milliseconds
volatile uint32_t lastDebounceTime = 0;
volatile uint32_t lastSmsDebounceTime = 0;

// Supabase
#define supabaseUrl "https://jursmglsfqaqrxvirtiw.supabase.co"
#define supabaseKey "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imp1cnNtZ2xzZnFhcXJ4dmlydGl3Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDQ3ODkxOTEsImV4cCI6MjA2MDM2NTE5MX0.ajGbf9fLrYAA0KXzYhGFCTju-d4h-iTYTeU5WfITj3k"
#define tableName "resident_number"

const char *ssid = "TK-gacura";
const char *password = "gisaniel924";

// Phone numbers and known IDs
std::vector<String> registeredPhoneNumbers;
std::vector<int> knownIds;

// Task handles
TaskHandle_t buttonTaskHandle = NULL;

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
  
  while(1) {
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

  // Create FreeRTOS tasks
  xTaskCreate(
    smsButtonTask,    // Task function
    "ButtonTask",      // Task name
    4096,                 // Stack size in words (more for JSON processing)
    NULL,                 // Task parameters
    1,                    // Priority (1 is low, configMAX_PRIORITIES-1 is high)
    &buttonTaskHandle     // Task handle
  );

  Serial.println("System ready! Press the button to fetch numbers from Supabase.");
}

void loop() {
  // Nothing to do here, FreeRTOS tasks handle everything
  vTaskDelay(portMAX_DELAY); // Just sleep
}
