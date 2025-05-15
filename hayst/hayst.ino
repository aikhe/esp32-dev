#include <WiFi.h>

// Replace with your actual network credentials
const char* ssid = "TK-gacura";
const char* password = "gisaniel924";

unsigned long disconnectTime = 10000;  // time to wait before disconnecting (10 sec)
unsigned long reconnectDelay = 5000;   // wait before trying to reconnect
unsigned long lastActionTime = 0;
bool isConnected = false;
bool hasDisconnected = false;

void setup() {
  Serial.begin(115200);
  delay(1000);  // Just to stabilize startup
  connectToWiFi();
  lastActionTime = millis();
}

void loop() {
  unsigned long currentTime = millis();

  if (isConnected && !hasDisconnected && (currentTime - lastActionTime >= disconnectTime)) {
    Serial.println("Disconnecting from WiFi...");
    WiFi.disconnect(true);
    hasDisconnected = true;
    isConnected = false;
    lastActionTime = currentTime;
  }

  if (hasDisconnected && (currentTime - lastActionTime >= reconnectDelay)) {
    Serial.println("Reconnecting to WiFi...");
    connectToWiFi();
    hasDisconnected = false;
    lastActionTime = currentTime;
  }
}

void connectToWiFi() {
  Serial.printf("Connecting to %s...\n", ssid);
  WiFi.begin(ssid, password);

  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 20) {
    delay(500);
    Serial.print(".");
    attempt++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    isConnected = true;
  } else {
    Serial.println("\nFailed to connect.");
    isConnected = false;
  }
}
