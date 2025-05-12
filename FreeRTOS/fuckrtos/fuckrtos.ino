#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_task_wdt.h>  // Include ESP32 Task Watchdog Timer header
#include <SPI.h>
#include <SD.h>
#include <Audio.h>

// WiFi credentials
const char* ssid = "TK-gacura";
const char* password = "gisaniel924";

Audio audio;

// microSD Card Reader connections
#define SD_CS_PIN    5
#define SD_SCK_PIN  18
#define SD_MISO_PIN 19
#define SD_MOSI_PIN 23

#define SD_CS          5
#define SPI_MOSI      23 
#define SPI_MISO      19
#define SPI_SCK       18

// I2S Connections (MAX98357)
#define I2S_DOUT      25
#define I2S_BCLK      27
#define I2S_LRC       26

// Constants
#define SOUND_SPEED 0.034  // Sound speed in cm/uS
#define DISTANCE_READ_INTERVAL 100  // ms
#define LED_UPDATE_INTERVAL 50      // ms
#define LCD_UPDATE_INTERVAL 500     // ms
#define LED_TIMEOUT_MINUTES 10      // LED stays on for 10 minutes
#define LED_TIMEOUT_MS (LED_TIMEOUT_MINUTES * 60 * 1000)  // 10 minutes in milliseconds
#define DEBOUNCE_TIME_MS 5000       // 5 seconds debounce to prevent rapid changes

// Global variables for sharing data between tasks
volatile float currentDistance = 0;
SemaphoreHandle_t distanceMutex;

// Timeout and state management - shared between LED and LCD
unsigned long stateLastChangeTime = 0;  // Shared timer for both LED and LCD
unsigned long lastStateUpdateTime = 0;  // For debounce
int lastDetectedRange = 0;  // 0=no detection, 1=far, 2=medium, 3=close
int activeState = 0;        // Current active state for both LED and LCD
bool timeoutEnabled = true;
// Additional variables for improved LCD management
unsigned long lastLCDUpdateTime = 0;  // Track last LCD update time
String currentDisplayedStatus = "";   // Track last status display to avoid unnecessary updates
unsigned long lcdLockStartTime = 0;   // When the LCD text was locked
bool lcdTextLocked = false;           // Whether LCD is showing locked text
#define LCD_LOCK_TIME_MS 5000         // 5 seconds to lock LCD text after level detection
#define LCD_UPDATE_INTERVAL 4000      // ms - longer interval for normal updates

// Task function prototypes
void readDistanceTask(void *parameter);
void controlOutputsTask(void *parameter);

// SMS message function prototypes
void createAlertSMS(float distance);
void createCriticalSMS(float distance);
void createWarningSMS(float distance);

// Supabase configuration
const char* supabaseUrl = "https://jursmglsfqaqrxvirtiw.supabase.co";
const char* supabaseKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imp1cnNtZ2xzZnFhcXJ4dmlydGl3Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDQ3ODkxOTEsImV4cCI6MjA2MDM2NTE5MX0.ajGbf9fLrYAA0KXzYhGFCTju-d4h-iTYTeU5WfITj3k";
const char* tableName = "resident_number";

const char* weatherApiKey = "7970309436bc52d518c7e71e314b8053";
const char* geminiApiKey = "AIzaSyD_g_WAsPqPKxltdOJt8VZw4uu359D3XXA";

// Fallback coordinates for STI College Fairview
float fallback_latitude = 14.676208;
float fallback_longitude = 121.043861;

// Global variables for weather data
float latitude = 0;  // Initialize with fallback coordinates
float longitude = 0;
String weatherDescription = "";  // Set default values
float temperature = 30.0;
float feelsLike = 32.0;
float humidity = 70.0;
String cityName = "Caloocan"; // Default city
bool weatherInitialized = false;
String aiWeatherMessage = "Sa kasalukuyan, walang banta ng baha sa Caloocan. Ang panahon ay maaliwalas, na may temperaturang 30.0°C, ngunit dahil sa 70% na halumigmig (humidity), mas ramdam and init na umaabot sa 32.0°C. Pinapayuhan ang lahat na magsuot ng magagaan at preskong damit at uminom ng maraming tubig upang makaiwas sa epekto ng matinding init.";

// SMS configuration
const char* smsFrom = "+639649687066"; // Your sender number or name
String phoneNumbers[20]; // Array to store up to 10 phone numbers
int numPhoneNumbers = 0;
char smsBody[1024]; // Buffer for dynamic SMS content

// Button configuration
#define BTTN_SMS 2                  // Button connected to GPIO pin 2
#define BTTN_AI 4
#define DEBOUNCE_DELAY 50          // Reduced debounce time to 50ms for faster response

// FreeRTOS handles
TaskHandle_t buttonTaskHandle = NULL;
TaskHandle_t wifiTaskHandle = NULL;
TaskHandle_t databaseTaskHandle = NULL;
TaskHandle_t weatherTaskHandle = NULL;
QueueHandle_t smsQueue = NULL;
SemaphoreHandle_t phoneNumbersMutex = NULL;
SemaphoreHandle_t weatherDataMutex = NULL;

// Recursively list a directory and its children
void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    for (uint8_t i = 0; i < levels; i++) {
      Serial.print("  ");
    }
    if (file.isDirectory()) {
      Serial.print("[DIR] ");
      Serial.println(file.name());
      // Recurse into sub-directory
      listDir(fs, file.name(), levels + 1);
    } else {
      Serial.print("[FILE] ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

bool getweather() {
  if (wifi.status() != wl_connected) {
    return false;
  }

  httpclient http;
  bool success = false;

  string url = string("http://api.openweathermap.org/data/2.5/weather?q=caloocan,ph&appid=") + weatherapikey + "&units=metric&lang=en";

  serial.println("weather api url: " + url);
  http.begin(url);
  int httpcode = http.get();

  if (httpcode == http_code_ok) {
    string payload = http.getstring();
    serial.println("openweather response: " + payload);

    staticjsondocument<1024> doc;
    deserializationerror error = deserializejson(doc, payload);

    if (!error) {
      string cityname = doc["name"].as<string>();
      location = cityname;

      string country = "unknown";
      if (doc.containskey("sys") && doc["sys"].containskey("country")) {
        country = doc["sys"]["country"].as<string>();
      }

      weatherdescription = "unknown";
      if (doc.containskey("weather") && doc["weather"][0].containskey("description")) {
        weatherdescription = doc["weather"][0]["description"].as<string>();
      }

      temperature = 0;
      feelslike = 0;
      humidity = 0;
      if (doc.containskey("main")) {
        temperature = doc["main"]["temp"].as<float>();
        feelslike = doc["main"]["feels_like"].as<float>();
        humidity = doc["main"]["humidity"].as<float>();
      }

      serial.println("==== weather information ====");
      serial.print("location: ");
      serial.print(cityname);
      serial.print(", ");
      serial.println(country);
      serial.print("weather: ");
      serial.println(weatherdescription);
      serial.print("temperature: ");
      serial.print(temperature);
      serial.println("°c");
      serial.print("feels like: ");
      serial.print(feelslike);
      serial.println("°c");
      serial.print("humidity: ");
      serial.print(humidity);
      serial.println("%");
      serial.println("============================");

      success = true;
    } else {
      serial.println("error parsing weather data");
    }
  } else {
    serial.println("failed to connect to openweather api, http code: " + string(httpcode));
  }
  http.end();

  return success;
}

bool getlocationfromipinfo() {
  if (wifi.status() != wl_connected) {
    return false;
  }

  httpclient http;
  bool success = false;

  http.begin("https://ipinfo.io/json");
  int httpcode = http.get();

  if (httpcode == http_code_ok) {
    string payload = http.getstring();
    serial.println("ipinfo response: " + payload);

    staticjsondocument<512> doc;
    deserializationerror error = deserializejson(doc, payload);

    if (!error && doc.containskey("loc")) {
      string loc = doc["loc"].as<string>();
      int commaindex = loc.indexof(',');

      if (commaindex > 0) {
        latitude = loc.substring(0, commaindex).tofloat();
        longitude = loc.substring(commaindex + 1).tofloat();

        string city = "unknown";
        string region = "unknown";
        string country = "unknown";

        if (doc.containskey("city")) {
          city = doc["city"].as<string>();
        }

        if (doc.containskey("region")) {
          region = doc["region"].as<string>();
        }

        if (doc.containskey("country")) {
          country = doc["country"].as<string>();
        }

        serial.print("detected location: ");
        serial.print(city);
        serial.print(", ");
        serial.print(region);
        serial.print(", ");
        serial.println(country);

        serial.print("coordinates from ipinfo.io: ");
        serial.print(latitude, 6);
        serial.print(", ");
        serial.println(longitude, 6);

        success = true;
      }
    }
  } else {
    serial.print("failed to get location from ipinfo, http code: ");
    serial.println(httpcode);
  }
  http.end();

  return success;
}

bool getAISuggestion() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Cannot fetch AI suggestion.");
    return false;
  }
  
  HTTPClient http;
  bool success = false;
  
  // Create a shorter prompt to reduce memory usage
  String prompt = "Weather update for " + cityName + ": " + weatherDescription + 
                  ", " + String(temperature, 1) + "°C (feels like " + 
                  String(feelsLike, 1) + "°C), humidity " + String(humidity, 0) + "%. ";
  
  prompt += "Write a 2-3 sentence message in Tagalog that: 1) Starts with 'PRAF Technology Weather Update:' ";
  prompt += "2) Includes flood risk assessment 3) Describes current weather 4) Gives a safety tip.";
  
  // Use a more memory-efficient approach with smaller JSON document
  DynamicJsonDocument requestDoc(1024);  // Reduced size
  JsonArray contents = requestDoc.createNestedArray("contents");
  JsonObject content = contents.createNestedObject();
  JsonArray parts = content.createNestedArray("parts");
  JsonObject part = parts.createNestedObject();
  part["text"] = prompt;
  
  JsonObject generationConfig = requestDoc.createNestedObject("generationConfig");
  generationConfig["temperature"] = 0.7;
  generationConfig["maxOutputTokens"] = 150;  // Reduced token count
  
  String requestBody;
  serializeJson(requestDoc, requestBody);
  requestDoc.clear();  // Free memory as soon as possible
  
  String geminiUrl = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=" + String(geminiApiKey);
  
  Serial.println("Sending AI request...");
  http.begin(geminiUrl);
  http.addHeader("Content-Type", "application/json");
  
  int httpCode = http.POST(requestBody);
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    // Create a new JSON document for parsing the response
    DynamicJsonDocument responseDoc(1024);  // Reduced size
    DeserializationError error = deserializeJson(responseDoc, payload);
    
    if (!error && responseDoc.containsKey("candidates") && 
        responseDoc["candidates"][0].containsKey("content") && 
        responseDoc["candidates"][0]["content"].containsKey("parts") &&
        responseDoc["candidates"][0]["content"]["parts"][0].containsKey("text")) {
      
      String aiResponse = responseDoc["candidates"][0]["content"]["parts"][0]["text"].as<String>();
      
      if (xSemaphoreTake(weatherDataMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        aiWeatherMessage = aiResponse;
        xSemaphoreGive(weatherDataMutex);
      }
      
      Serial.println("\n==== AI WEATHER SUGGESTION ====");
      Serial.println(aiWeatherMessage);
      Serial.println("===============================\n");
      
      success = true;
    } else {
      Serial.println("Error parsing Gemini API response");
    }
    responseDoc.clear();  // Free memory as soon as possible
  } else {
    Serial.print("Failed to connect to Gemini API, HTTP code: ");
    Serial.println(httpCode);
  }
  
  http.end();
  return success;
}

// Button monitoring task
void buttonTask(void *pvParameters) {
  // Configure task so that it doesn't use the watchdog
  esp_task_wdt_delete(NULL); // Remove current task from WDT watch
  
  int lastButtonState = HIGH;
  unsigned long lastDebounceTime = 0;
  
  while (1) {
    int reading = digitalRead(BTTN_SMS);
    int ai = digitalRead(BTTN_AI);
    
    // If button state changed
    if (reading != lastButtonState) {
      lastDebounceTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    }

    
    // If enough time has passed since last state change
    if ((xTaskGetTickCount() * portTICK_PERIOD_MS - lastDebounceTime) > DEBOUNCE_DELAY) {
      // If button is pressed (LOW)
      if (ai == LOW) {
        Serial.println("Button pressed! AI SMS...");
        getAISuggestion();
        Serial.println("AI suggestion fetch successful!");
        Serial.println(aiWeatherMessage);

        while (digitalRead(BTTN_AI) == LOW) {
          vTaskDelay(pdMS_TO_TICKS(10));
        }
      }

      if (reading == LOW) {
        Serial.println("Button pressed! Queueing SMS...");
        // Send a message to the queue
        audio.connecttoFS(SD, "DEVICE-START-VOICE.mp3");
        while (audio.isRunning()) {
          audio.loop();
        }
        
        // Wait for button release to prevent multiple triggers
        while (digitalRead(BTTN_SMS) == LOW) {
          vTaskDelay(pdMS_TO_TICKS(10));
        }
      }
    }
    
    lastButtonState = reading;
    vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to prevent task starvation
  }
}

void setup() {
  digitalWrite(SD_CS, HIGH);

  Serial.begin(115200);

  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial.print(".");
  }
  Serial.println(" CONNECTED");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Set microSD Card CS as OUTPUT and set HIGH
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  // Initialize SPI bus for microSD Card
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  // Initialize microSD card with custom SPI
  if (!SD.begin(SD_CS, SPI)) {
    Serial.println("Error accessing microSD card!");
    while (true)
      ;
  }

  Serial.println("microSD card initialized.");

  // Setup I2S
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

  // Set Volume (0 to 21)
  audio.setVolume(100);

  pinMode(BTTN_SMS, INPUT_PULLUP);
  pinMode(BTTN_AI, INPUT_PULLUP);

  listDir(SD, "/", 0);

  getAISuggestion();
  Serial.println("AI suggestion fetch successful!");
  getLocationFromIpInfo();
  Serial.println("AI suggestion fetch successful!");
  getWeather();
  Serial.println("Weather data fetch successful!");

  xTaskCreate(
    buttonTask,      // Task function
    "ButtonTask",    // Task name
    2048,           // Stack size
    NULL,           // Task parameters
    1,              // Task priority
    &buttonTaskHandle
  );
}

void loop() {
  // Empty loop as tasks are handled by FreeRTOS
  vTaskDelay(pdMS_TO_TICKS(1000));
}
