#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char *ssid = "TK-gacura";
const char *password = "gisaniel924";
const char *weatherApiKey = "7970309436bc52d518c7e71e314b8053";
const char *geminiApiKey = "AIzaSyD_g_WAsPqPKxltdOJt8VZw4uu359D3XXA";

// Fallback coordinates for STI College Fairview
float fallback_latitude = 14.7160;
float fallback_longitude = 121.0615;

float latitude = 0.0;
float longitude = 0.0;

String weatherDescription = "";
float temperature = 0.0;
float feelsLike = 0.0;
float humidity = 0.0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(1000);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Get location from ipinfo.io
    if (getLocationFromIpInfo()) {
      Serial.println("Successfully obtained location from ipinfo.io");
    } else {
      latitude = fallback_latitude;
      longitude = fallback_longitude;
      Serial.println("Using fallback coordinates");
    }
  } else {
    Serial.println("\nFailed to connect to WiFi");
  }
}

void loop() {
  // Update weather and AI suggestion every hour (3600000 ms)
  static unsigned long lastUpdate = 0;
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastUpdate >= 3600000 || lastUpdate == 0) {
    lastUpdate = currentMillis;
    
    if (WiFi.status() == WL_CONNECTED) {
      if (getWeather()) {
        getAISuggestion();
      }
    }
  }
  
  delay(1000);
}

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

bool getWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  
  HTTPClient http;
  bool success = false;

  String url = "http://api.openweathermap.org/data/2.5/weather?lat=" + String(latitude, 6) + "&lon=" + String(longitude, 6) + "&appid=" + weatherApiKey + "&units=metric&lang=en";

  Serial.println("Weather API URL: " + url);
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("OpenWeather Response: " + payload);

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      String cityName = doc["name"].as<String>();
      String country = "unknown";
      if (doc.containsKey("sys") && doc["sys"].containsKey("country")) {
        country = doc["sys"]["country"].as<String>();
      }

      weatherDescription = "unknown";
      if (doc.containsKey("weather") && doc["weather"][0].containsKey("description")) {
        weatherDescription = doc["weather"][0]["description"].as<String>();
      }

      temperature = 0;
      feelsLike = 0;
      humidity = 0;
      if (doc.containsKey("main")) {
        temperature = doc["main"]["temp"].as<float>();
        feelsLike = doc["main"]["feels_like"].as<float>();
        humidity = doc["main"]["humidity"].as<float>();
      }

      Serial.println("==== WEATHER INFORMATION ====");
      Serial.print("Location: ");
      Serial.print(cityName);
      Serial.print(", ");
      Serial.println(country);
      Serial.print("Weather: ");
      Serial.println(weatherDescription);
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.println("°C");
      Serial.print("Feels like: ");
      Serial.print(feelsLike);
      Serial.println("°C");
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.println("%");
      Serial.println("============================");
      
      success = true;
    } else {
      Serial.println("Error parsing weather data");
    }
  } else {
    Serial.println("Failed to connect to OpenWeather API, HTTP code: " + String(httpCode));
  }
  http.end();
  
  return success;
}

void getAISuggestion() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  
  HTTPClient http;
  
  String prompt = "Provide a short and helpful suggestion to inform residents about the current weather and keep them safe.\n\n";
  prompt += "- Weather Details:\n";
  prompt += "  - Weather: " + weatherDescription + "\n";
  prompt += "  - Temperature: " + String(temperature, 2) + "°C\n";
  prompt += "  - Feels like: " + String(feelsLike, 2) + "°C\n";
  prompt += "  - Humidity: " + String(humidity, 2) + "%\n\n";
  prompt += "Instructions:\n";
  prompt += "- Write the message like a weather forecast-casual, clear, and understandable for most people.\n";
  prompt += "- Start with: \"PRAF Technology Weather Update:\".\n";
  prompt += "- The message should be one sentence long and include a note that it's from PRAF Technology.\n";
  prompt += "- If the weather poses a flood risk, alert the residents.\n";
  prompt += "- If flooding is unlikely, suggest a safe way to deal with the weather while reassuring them.\n";
  prompt += "- Maintain a formal tone and avoid AI-like phrasing.\n";
  prompt += "- Do not use uncertain words like \"naman.\"\n";
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
        responseDoc["candidates"][0].containsKey("content") && 
        responseDoc["candidates"][0]["content"].containsKey("parts") &&
        responseDoc["candidates"][0]["content"]["parts"][0].containsKey("text")) {
      
      String aiMessage = responseDoc["candidates"][0]["content"]["parts"][0]["text"].as<String>();
      
      Serial.println("\n==== AI WEATHER SUGGESTION ====");
      Serial.println(aiMessage);
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
