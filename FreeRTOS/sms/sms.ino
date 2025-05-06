/*
 * ESP32 Button SMS Sender
 * 
 * This program uses an ESP32 to send SMS messages when a button is pressed
 * Uses the HttpSMS API for message delivery
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "TK-gacura";
const char* password = "gisaniel924";

// HttpSMS API key
const char* httpSmsApiKey = "MNJmgF7kRvUrTfj4fqDUbrzwoVFpMToWdTbiUx3sQ6jreYnbnu7bym-rQG3kB8_U";

// SMS configuration
const char* smsFrom = "+639649687066"; // Your sender number or name
const char* smsTo = "+639099082245";     // Recipient phone number with country code
const char* smsBody = 
  "🚨 FLOOD ALERT! 🚨\n\n"
  "📱 System Check:\n\n"
  "📍 Location: Caloocan\n"
  "🌤️ Weather: few clouds\n"
  "🌡️ Temperature: 31.5°C\n"
  "🌡️ Feels like: 34.2°C\n"
  "💧 Humidity: 53%\n\n"
  "🤖 AI Weather Update:\n"
  "Ayon sa pinakabagong update ng PRAF Technology: Sa kasalukuyan, walang banta ng baha sa Caloocan. "
  "Bahagyang Makulimlim ang panahon, na may temperaturang 31.55°C, ngunit dahil sa 53% na halumigmig (humidity), "
  "mas ramdam and init na umaabot sa 34.24°C. Pinapayuhan ang lahat na magsuot ng magagaan at preskong damit "
  "at uminom ng maraming tubig upang makaiwas sa epekto ng matinding init.\n\n"
  "From: PRAF Technology";

// Button configuration
#define BTTN_SMS 2                  // Button connected to GPIO pin 2
unsigned long lastDebounceTime = 0; // Last time the button was pressed
unsigned long debounceDelay = 300;  // Debounce time in milliseconds
int buttonState = HIGH;             // Current state of the button
int lastButtonState = HIGH;         // Previous state of the button

// Function to send an SMS via HTTP
void sendHttpSMS(const char* from, const char* to, const char* body) {
  Serial.println("Preparing to send SMS...");
  WiFiClientSecure client;
  client.setInsecure();
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
  client.println(httpSmsApiKey);
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(jsonPayload.length());
  client.println("Connection: close");
  client.println();
  client.println(jsonPayload);
  Serial.println("SMS Request sent!");
  // Read and print the response
  Serial.println("Reading SMS API response:");
  while (client.connected() || client.available()) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
    }
  }
  client.stop();
  Serial.println("SMS Connection closed");
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nESP32 Button SMS Sender");
  
  // Set button pin as input with internal pull-up resistor
  pinMode(BTTN_SMS, INPUT_PULLUP);
  
  // Connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  Serial.println("Press the button connected to GPIO 2 to send an SMS");
}

void loop() {
  // Read the state of the button
  int reading = digitalRead(BTTN_SMS);
  
  // If the button state changed, reset the debouncing timer
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  // Check if the button state has been stable for longer than the debounce delay
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // If the button state has changed
    if (reading != buttonState) {
      buttonState = reading;
      
      // If the button is pressed (LOW because of the pull-up resistor)
      if (buttonState == LOW) {
        Serial.println("Button pressed! Sending SMS...");
        sendHttpSMS(smsFrom, smsTo, smsBody);
      }
    }
  }
  
  // Save the current reading for the next loop
  lastButtonState = reading;
}
