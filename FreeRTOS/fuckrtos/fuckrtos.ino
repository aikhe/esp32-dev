/*
 * ESP32 MP3 Player with Supabase Database Fetching using FreeRTOS
 * 
 * This program plays MP3 files from an SD card while simultaneously 
 * fetching data from a Supabase database and displaying it on the serial monitor.
 * Each API call retrieves a different row from the database.
 * 
 * Hardware:
 * - ESP32
 * - PCM5102A I2S DAC
 * - microSD Card Reader
 */

#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"
#include "SPI.h"
#include "SD.h"
#include "FS.h"
#include "HTTPClient.h"
#include "ArduinoJson.h"

// microSD Card Reader connections
#define SD_CS       5
#define SPI_MOSI    23
#define SPI_MISO    19
#define SPI_SCK     18

// I2S Connections (PCM5102A)
#define I2S_BCLK    27
#define I2S_LRC     26
#define I2S_DOUT    25

// WiFi credentials
const char* ssid = "TK-gacura";
const char* password = "gisaniel924";

// Supabase configuration
const char* supabaseUrl = "https://jursmglsfqaqrxvirtiw.supabase.co";
const char* supabaseKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imp1cnNtZ2xzZnFhcXJ4dmlydGl3Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDQ3ODkxOTEsImV4cCI6MjA2MDM2NTE5MX0.ajGbf9fLrYAA0KXzYhGFCTju-d4h-iTYTeU5WfITj3k";
const char* tableName = "resident_number";

// Audio instance
Audio audio;

// Task handles
TaskHandle_t audioTaskHandle = NULL;
TaskHandle_t apiTaskHandle = NULL;

// Maximum number of records to request at once
const int MAX_RECORDS = 10;
// Track which record index we're currently on
int currentRecordIndex = 0;
// Store fetched records to cycle through them
DynamicJsonDocument records(8192); // Larger buffer for multiple records

// Semaphore to protect serial communication
SemaphoreHandle_t xSerialSemaphore;

// Function prototypes
void audioTask(void *parameter);
void apiTask(void *parameter);
void fetchSupabaseRecords();

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  
  // Increase clock speed for better performance
  setCpuFrequencyMhz(240);
  
  // Create mutex for serial communication
  xSerialSemaphore = xSemaphoreCreateMutex();
  
  // Initialize SPI bus for SD card
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  
  // Initialize SD card
  if(xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
    Serial.println("Initializing SD card...");
    xSemaphoreGive(xSerialSemaphore);
  }
  
  if (!SD.begin(SD_CS)) {
    if(xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
      Serial.println("SD card initialization failed!");
      xSemaphoreGive(xSerialSemaphore);
    }
    while(1);
  }
  
  if(xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
    Serial.println("SD card initialized successfully!");
    xSemaphoreGive(xSerialSemaphore);
  }
  
  // Initialize WiFi
  WiFi.begin(ssid, password);
  
  if(xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
    Serial.println("Connecting to WiFi...");
    xSemaphoreGive(xSerialSemaphore);
  }
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if(xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
      Serial.print(".");
      xSemaphoreGive(xSerialSemaphore);
    }
  }
  
  if(xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
    Serial.println("");
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    xSemaphoreGive(xSerialSemaphore);
  }
  
  // Initialize I2S audio
  if(xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
    Serial.println("Initializing I2S audio...");
    xSemaphoreGive(xSerialSemaphore);
  }
  
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(21); // 0...21
  
  if(xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
    Serial.println("I2S audio initialized!");
    xSemaphoreGive(xSerialSemaphore);
  }
  
  // Create tasks with higher stack sizes for HTTPS connections
  xTaskCreatePinnedToCore(
    audioTask,        // Task function
    "AudioTask",      // Name of task
    10000,            // Stack size of task
    NULL,             // Parameter of the task
    1,                // Priority of the task
    &audioTaskHandle, // Task handle
    0                 // Core where the task should run
  );
  
  xTaskCreatePinnedToCore(
    apiTask,          // Task function
    "ApiTask",        // Name of task
    16000,            // Stack size of task - increased for HTTPS/SSL
    NULL,             // Parameter of the task
    1,                // Priority of the task
    &apiTaskHandle,   // Task handle
    1                 // Core where the task should run
  );
}

void loop() {
  // Main loop is empty as tasks handle the work
  vTaskDelay(1000 / portTICK_PERIOD_MS); // Just to avoid watchdog triggers
}

// Audio Task - Handles MP3 playback
void audioTask(void *parameter) {
  while(1) {
    // Check if SD card contains an MP3 file (replace with your file name)
    if(SD.exists("/DEVICE-START-VOICE.mp3")) {
      if(xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
        Serial.println("Playing MP3 file from SD card...");
        xSemaphoreGive(xSerialSemaphore);
      }
      
      audio.connecttoFS(SD, "/DEVICE-START-VOICE.mp3");
      
      // Audio loop
      while(audio.isRunning()) {
        audio.loop();
        vTaskDelay(1); // Yield to other tasks
      }
      
      if(xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
        Serial.println("MP3 playback finished. Waiting before replaying...");
        xSemaphoreGive(xSerialSemaphore);
      }
      
      vTaskDelay(5000 / portTICK_PERIOD_MS); // Wait 5 seconds before replaying
    } else {
      if(xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
        Serial.println("MP3 file not found on SD card!");
        Serial.println("Please make sure you have a file named 'DEVICE-START-VOICE.mp3' on your SD card.");
        xSemaphoreGive(xSerialSemaphore);
      }
      
      vTaskDelay(10000 / portTICK_PERIOD_MS); // Check again after 10 seconds
    }
  }
}

// API Task - Fetches data from Supabase
void apiTask(void *parameter) {
  // Add a delay to ensure audio task starts first and establishes its resources
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  
  bool initialFetch = true;
  
  while(1) {
    if(WiFi.status() == WL_CONNECTED) {
      // Only fetch new batch of records if we've gone through all current ones
      // or on initial startup
      if(currentRecordIndex == 0 || initialFetch) {
        fetchSupabaseRecords();
        initialFetch = false;
      }
      
      // Display the current record if we have records
      int recordCount = records.size();
      if(recordCount > 0) {
        if(xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
          Serial.println("====== SUPABASE RECORD ======");
          Serial.print("Record index: ");
          Serial.print(currentRecordIndex + 1);
          Serial.print(" of ");
          Serial.println(recordCount);
          
          // Print all fields in the current record
          JsonObject currentRecord = records[currentRecordIndex].as<JsonObject>();
          for (JsonPair kv : currentRecord) {
            Serial.print("  ");
            Serial.print(kv.key().c_str());
            Serial.print(": ");
            
            // Handle different value types
            if (kv.value().is<const char*>()) {
              Serial.println(kv.value().as<const char*>());
            } else if (kv.value().is<int>()) {
              Serial.println(kv.value().as<int>());
            } else if (kv.value().is<float>()) {
              Serial.println(kv.value().as<float>());
            } else if (kv.value().is<bool>()) {
              Serial.println(kv.value().as<bool>() ? "true" : "false");
            } else {
              Serial.println("[complex type]");
            }
          }
          Serial.println("=============================");
          xSemaphoreGive(xSerialSemaphore);
        }
        
        // Move to next record for next time
        currentRecordIndex = (currentRecordIndex + 1) % recordCount;
        
        // If we're back at the start, fetch new data next time
        // (handled by the condition at the top of the loop)
      }
    } else {
      if(xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
        Serial.println("WiFi not connected. Reconnecting...");
        xSemaphoreGive(xSerialSemaphore);
      }
      
      WiFi.reconnect();
    }
    
    // Wait before displaying the next record or fetching more
    vTaskDelay(15000 / portTICK_PERIOD_MS); // Every 15 seconds
  }
}

// Helper function to fetch records from Supabase
void fetchSupabaseRecords() {
  if(WiFi.status() != WL_CONNECTED) {
    return;
  }
  
  HTTPClient http;
  
  if(xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
    Serial.println("Fetching data from Supabase...");
    xSemaphoreGive(xSerialSemaphore);
  }
  
  // Create Supabase API endpoint URL
  String apiUrl = String(supabaseUrl) + "/rest/v1/" + String(tableName);
  
  // Add query parameters to limit results and get random order
  apiUrl += "?select=*&limit=" + String(MAX_RECORDS);
  
  // Order randomly to get different records each fetch
  // Note: This assumes your table has a created_at column.
  // For truly random order, you might need to use a different approach
  // or add a random field to your database.
  apiUrl += "&order=random()";
  
  if(xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
    Serial.print("API URL: ");
    Serial.println(apiUrl);
    xSemaphoreGive(xSerialSemaphore);
  }
  
  http.begin(apiUrl);
  
  // Add required Supabase headers
  http.addHeader("apikey", supabaseKey);
  http.addHeader("Authorization", "Bearer " + String(supabaseKey));
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Prefer", "return=representation");
  
  // Send HTTP GET request
  int httpResponseCode = http.GET();
  
  if (httpResponseCode > 0) {
    String payload = http.getString();
    
    // Clear previous records
    records.clear();
    
    // Parse JSON response - expecting an array of records
    DeserializationError error = deserializeJson(records, payload);
    
    if (!error) {
      int recordCount = records.size();
      
      if(xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
        Serial.print("Successfully fetched ");
        Serial.print(recordCount);
        Serial.println(" records from Supabase");
        xSemaphoreGive(xSerialSemaphore);
      }
      
      currentRecordIndex = 0; // Reset current index to start at first record
    } else {
      if(xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
        Serial.print("JSON parsing error: ");
        Serial.println(error.c_str());
        Serial.println("Response payload:");
        Serial.println(payload);
        xSemaphoreGive(xSerialSemaphore);
      }
    }
  } else {
    if(xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
      Serial.print("Error in Supabase API request. Error code: ");
      Serial.println(httpResponseCode);
      xSemaphoreGive(xSerialSemaphore);
    }
  }
  
  http.end();
}

// Optional: Callbacks for the Audio library
void audio_info(const char *info) {
  if(xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
    Serial.print("audio_info: ");
    Serial.println(info);
    xSemaphoreGive(xSerialSemaphore);
  }
}

void audio_id3data(const char *info) { // ID3 metadata
  if(xSemaphoreTake(xSerialSemaphore, portMAX_DELAY)) {
    Serial.print("id3data: ");
    Serial.println(info);
    xSemaphoreGive(xSerialSemaphore);
  }
}
