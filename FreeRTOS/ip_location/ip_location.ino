#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// WiFi credentials
const char* ssid = "someday";
const char* password = "woainilumi34";

// Pin definitions
const int buttonPin = 2;  // Button connected to GPIO 2

// Global variables for location data
float latitude = 0.0;
float longitude = 0.0;

// Queue handle for button press events
QueueHandle_t buttonEventQueue;

// Function to get location from IP info
bool getLocationFromIpInfo() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  HTTPClient http;
  bool success = false;
  http.begin("https://ipinfo.io/json");
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("IPInfo Response: " + payload);
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (!error && doc.containsKey("loc")) {
      String loc = doc["loc"].as<String>();
      int commaIndex = loc.indexOf(',');
      if (commaIndex > 0) {
        latitude = loc.substring(0, commaIndex).toFloat();
        longitude = loc.substring(commaIndex + 1).toFloat();
        String city = "Unknown";
        String region = "Unknown";
        String country = "Unknown";
        if (doc.containsKey("city")) {
          city = doc["city"].as<String>();
        }
        if (doc.containsKey("region")) {
          region = doc["region"].as<String>();
        }
        if (doc.containsKey("country")) {
          country = doc["country"].as<String>();
        }
        Serial.print("Detected Location: ");
        Serial.print(city);
        Serial.print(", ");
        Serial.print(region);
        Serial.print(", ");
        Serial.println(country);
        Serial.print("Coordinates from ipinfo.io: ");
        Serial.print(latitude, 6);
        Serial.print(", ");
        Serial.println(longitude, 6);
        success = true;
      }
    }
  } else {
    Serial.print("Failed to get location from IPInfo, HTTP code: ");
    Serial.println(httpCode);
  }
  http.end();
  return success;
}

// Button interrupt service routine
void IRAM_ATTR buttonISR() {
  uint32_t eventValue = 1;
  xQueueSendFromISR(buttonEventQueue, &eventValue, NULL);
}

// Button handler task
void buttonHandlerTask(void *parameter) {
  uint32_t eventValue;
  
  while (true) {
    if (xQueueReceive(buttonEventQueue, &eventValue, portMAX_DELAY)) {
      Serial.println("Button pressed! Getting location...");
      
      // Attempt to get location
      if (getLocationFromIpInfo()) {
        Serial.println("Successfully obtained location");
      } else {
        Serial.println("Failed to get location");
      }
    }
    
    // Small delay to avoid repeated triggers
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 Button IP Location Demo");
  
  // Initialize button with pull-up resistor
  pinMode(buttonPin, INPUT_PULLUP);
  
  // Create queue for button events
  buttonEventQueue = xQueueCreate(10, sizeof(uint32_t));
  
  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Attach interrupt to button pin
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonISR, FALLING);
  
  // Create button handler task
  xTaskCreate(
    buttonHandlerTask,
    "ButtonHandler",
    4096,     // Stack size (might need to be adjusted)
    NULL,     // Parameters
    1,        // Priority
    NULL      // Task handle
  );
}

void loop() {
  // Main loop is empty as everything is handled by FreeRTOS tasks
  delay(1000);
}
