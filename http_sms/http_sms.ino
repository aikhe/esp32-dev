#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

const char* ssid = "TK-gacura";
const char* password = "gisaniel924";

// HttpSMS API credentials
const char* apiKey = "iFqOahA-gXvOzLHlt3mHWIs5kLsqQ11FFu8QblKwxKMzDj49mLyw_dpEgMkIDFsS";
const char* fromNumber = "+639649687066";
const char* toNumber = "+639649687066";
const char* messageBody = "Hello, this is a test message from my ESP32!";

WiFiClientSecure client;

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Insecure mode (for testing only)
  client.setInsecure();

  // Send SMS using HttpSMS
  sendHttpSMS(fromNumber, toNumber, messageBody);
}

void loop() {
  // Do nothing
}

void sendHttpSMS(const char* from, const char* to, const char* body) {
  Serial.println("Preparing to send SMS...");
  
  if (!client.connect("api.httpsms.com", 443)) {
    Serial.println("Connection to HttpSMS API failed");
    return;
  }
  
  // Create JSON payload
  DynamicJsonDocument doc(1024);
  doc["content"] = body;
  doc["from"] = from;
  doc["to"] = to;
  
  String jsonPayload;
  serializeJson(doc, jsonPayload);
  
  // Send POST request
  client.println("POST /v1/messages/send HTTP/1.1");
  client.println("Host: api.httpsms.com");
  client.print("x-api-key: ");
  client.println(apiKey);
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(jsonPayload.length());
  client.println("Connection: close");
  client.println();
  client.println(jsonPayload);
  
  Serial.println("Request sent!");
  
  // Read and print the response
  Serial.println("Reading response:");
  while (client.connected() || client.available()) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
    }
  }
  
  client.stop();
  Serial.println("Connection closed");
}
