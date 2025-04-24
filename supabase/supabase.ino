#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define SSID              "TK-gacura"
#define PASSWORD          "gisaniel924"

#define supabaseUrl       "https://jursmglsfqaqrxvirtiw.supabase.co"
#define supabaseKey       "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imp1cnNtZ2xzZnFhcXJ4dmlydGl3Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDQ3ODkxOTEsImV4cCI6MjA2MDM2NTE5MX0.ajGbf9fLrYAA0KXzYhGFCTju-d4h-iTYTeU5WfITj3k"
#define tableName         "resident_number"

// Variables to track database state
int lastKnownMaxId = 0;
unsigned long lastCheckTime = 0;
const unsigned long checkInterval = 5000; // Check every 5 seconds

void setup() {
  Serial.begin(115200);

  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi\n");

  // Get all entries and find the maximum ID
  getInitialEntries();
}

void loop() {
  // Check for new entries periodically
  unsigned long currentTime = millis();
  if (currentTime - lastCheckTime >= checkInterval) {
    lastCheckTime = currentTime;
    checkForNewEntries();
  }
}

void getInitialEntries() {
  if (WiFi.status() != WL_CONNECTED) {
    reconnectWiFi();
  }

  HTTPClient http;
  // Get all entries and order by ID
  String endpoint = String(supabaseUrl) + "/rest/v1/" + tableName + "?order=id.desc";
  
  http.begin(endpoint);
  http.addHeader("apikey", supabaseKey);
  http.addHeader("Authorization", "Bearer " + String(supabaseKey));
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("Initial entries received:");
    
    // Parse JSON response
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
      Serial.print("JSON deserialization failed: ");
      Serial.println(error.c_str());
    } else {
      // Process entries and find the highest ID
      JsonArray array = doc.as<JsonArray>();
      
      if (array.size() > 0) {
        // Get the highest ID (first entry since we ordered by desc)
        lastKnownMaxId = array[0]["id"].as<int>();
        
        Serial.print("Found ");
        Serial.print(array.size());
        Serial.println(" entries.");
        Serial.print("Highest ID found: ");
        Serial.println(lastKnownMaxId);
        
        // Print all entries
        for (JsonVariant entry : array) {
          int id = entry["id"];
          String number = entry["number"].as<String>();
          
          Serial.print("ID: ");
          Serial.print(id);
          Serial.print(", Number: ");
          Serial.println(number);
        }
      } else {
        Serial.println("No entries found in the database.");
      }
    }
  } else {
    Serial.print("Error getting entries. HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.println("Response: " + response);
  }
  
  http.end();
}

void checkForNewEntries() {
  if (WiFi.status() != WL_CONNECTED) {
    reconnectWiFi();
  }

  HTTPClient http;
  // Only query for entries with ID greater than our last known max ID
  String endpoint = String(supabaseUrl) + "/rest/v1/" + tableName + "?id=gt." + lastKnownMaxId + "&order=id.asc";
  
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
      
      if (array.size() > 0) {
        Serial.println("\n=== New Entries Detected! ===");
        
        // Process new entries
        for (JsonVariant entry : array) {
          int id = entry["id"];
          String number = entry["number"].as<String>();
          
          // Update our last known max ID if this one is higher
          if (id > lastKnownMaxId) {
            lastKnownMaxId = id;
          }
          
          Serial.print("New Entry - ID: ");
          Serial.print(id);
          Serial.print(", Number: ");
          Serial.println(number);
        }
        
        Serial.println("============================\n");
      }
    }
  } else {
    Serial.print("Error checking for new entries. HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.println("Response: " + response);
  }
  
  http.end();
}

void reconnectWiFi() {
  Serial.println("WiFi not connected. Attempting to reconnect...");
  while (!WiFi.reconnect()) {
    Serial.println("Reconnecting to WiFi...");
    delay(500);
  }
  Serial.println("WiFi reconnected.");
}
