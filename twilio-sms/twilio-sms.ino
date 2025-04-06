#include <WiFi.h>
#include <WiFiClientSecure.h>

const char* ssid = "TK-gacura";
const char* password = "gisaniel924";

const char* accountSid = "";
const char* authToken = "";

const char* fromNumber = "+13023054782";
const char* toNumber = "+639649687066";
const char* messageBody = "Text I want to send";

WiFiClientSecure client;

String base64Encode(String input) {
  const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  String encoded = "";
  int val = 0, valb = -6;
  for (unsigned char c : input) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      encoded += chars[(val >> valb) & 0x3F];
      valb -= 6;
    }
  }
  if (valb > -6) encoded += chars[((val << 8) >> (valb + 8)) & 0x3F];
  while (encoded.length() % 4) encoded += '=';
  return encoded;
}

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

  // Send SMS
  sendSMS(toNumber, messageBody);
}

void loop() {
  // Do nothing
}

void sendSMS(const char* to, const char* body) {
  if (!client.connect("api.twilio.com", 443)) {
    Serial.println("Connection to Twilio failed");
    return;
  }

  String url = "/2010-04-01/Accounts/" + String(accountSid) + "/Messages.json";
  String credentials = String(accountSid) + ":" + String(authToken);
  String authHeader = "Basic " + base64Encode(credentials);

  String postData = "From=" + String(fromNumber) + "&To=" + String(to) + "&Body=" + String(body);

  // Send POST request
  client.println("POST " + url + " HTTP/1.1");
  client.println("Host: api.twilio.com");
  client.println("Authorization: " + authHeader);
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.print("Content-Length: ");
  client.println(postData.length());
  client.println();
  client.print(postData);

  // Read the response
  while (client.connected()) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
    }
  }

  client.stop();
}
