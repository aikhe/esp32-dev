#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char *ssid = "TK-gacura";
const char *password = "gisaniel924";
const char *weatherApiKey = "7970309436bc52d518c7e71e314b8053";

// Fallback coordinates for STI College Fairview
float fallback_latitude = 14.7160;
float fallback_longitude = 121.0615;

float latitude = 0.0;
float longitude = 0.0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.print("Connecting to WiFi");
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
      Serial.println("Using fallback coordinates for Caloocan City");
    }

    // Get weather based on coordinates
    getWeather();
  } else {
    Serial.println("\nFailed to connect to WiFi");
  }
}

void loop() {
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

void getWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Request weather with the coordinates
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

        String weather = "unknown";
        if (doc.containsKey("weather") && doc["weather"][0].containsKey("description")) {
          weather = doc["weather"][0]["description"].as<String>();
        }

        float temp = 0;
        float feelsLike = 0;
        float humidity = 0;
        if (doc.containsKey("main")) {
          temp = doc["main"]["temp"].as<float>();
          feelsLike = doc["main"]["feels_like"].as<float>();
          humidity = doc["main"]["humidity"].as<float>();
        }

        Serial.println("==== WEATHER INFORMATION ====");
        Serial.print("Location: ");
        Serial.print(cityName);
        Serial.print(", ");
        Serial.println(country);
        Serial.print("Weather: ");
        Serial.println(weather);
        Serial.print("Temperature: ");
        Serial.print(temp);
        Serial.println("°C");
        Serial.print("Feels like: ");
        Serial.print(feelsLike);
        Serial.println("°C");
        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.println("%");
        Serial.println("============================");
      } else {
        Serial.println("Error parsing weather data");
      }
    } else {
      Serial.println("Failed to connect to OpenWeather API, HTTP code: " + String(httpCode));
    }
    http.end();
  }
}
