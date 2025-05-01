#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

// ----------- Definitions ------------
#define LED_ONE 32
#define LED_TWO 33
#define LED_THREE 25
#define BTTN_AI 4

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

// LCD I2C init (16x2)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------- AI Suggestion Function ----------
void getAISuggestion() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;

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

  http.begin(geminiUrl);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(requestBody);

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
    }
  } else {
    Serial.print("Failed to connect to Gemini API, HTTP code: ");
    Serial.println(httpCode);
    Serial.println("Request Body: " + requestBody);
  }

  http.end();
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
  String paddedMsg = msg + "   ";  // 5 spaces on the right only
  paddedMsg = paddedMsg + paddedMsg.substring(0, 16);  // Wrap for smooth scroll

  int len = paddedMsg.length();
  int pos = (len - 16);  // Start from the end
  const int interval = 50;
  unsigned long lastUpdate = 0;

  while (true) {
    if (millis() - lastUpdate >= interval) {
      String scrollSegment = paddedMsg.substring(pos, pos + 16);

      lcd.setCursor(0, 0);
      lcd.print(scrollSegment);

      lcd.setCursor(0, 1);
      lcd.print(scrollSegment);

      // Move backward (left to right visually)
      pos = (pos - 1 + (len - 16)) % (len - 16);

      lastUpdate = millis();
    }
    vTaskDelay(1);
  }
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);

  pinMode(LED_ONE, OUTPUT);
  pinMode(LED_TWO, OUTPUT);
  pinMode(LED_THREE, OUTPUT);

  // LCD Init
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("System Booting...");

  // Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PRAF System Ready");

  // Tasks
  xTaskCreatePinnedToCore(waveLEDTask, "LED Wave", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(aiButtonTask, "AI Button", 8192, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(scrollLCDTask, "LCD Scroll", 2048, NULL, 1, NULL, 1);
}

void loop() {
  // Empty loop, tasks handle everything
}
