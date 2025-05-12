#include "Arduino.h"
#include "Audio.h"
#include "SD.h"
#include "FS.h"
#include "SPI.h"
#include <WiFi.h>
#include <ArduinoJson.h>

// WiFi credentials - update with your own
const char* ssid = "TK-gacura";
const char* password = "gisaniel924";

// microSD Card Reader connections
#define SD_CS          5
#define SPI_MOSI      23 
#define SPI_MISO      19
#define SPI_SCK       18

// I2S Connections (MAX98357)
#define I2S_DOUT      25
#define I2S_BCLK      27
#define I2S_LRC       26

// Create Audio object
Audio audio;

// AI suggestion variables
String aiSuggestion = ""; // Store the AI suggestion

// Predefined collection of fun facts about music
const char* musicFacts[] = {
  "Sound travels slower than light.",
  "Mozart wrote music at age five.",
  "Vinyl records are making a comeback.",
  "MP3 compression reduces files by 90%.",
  "Most orchestras tune to A440Hz.",
  "Whales can create complex songs.",
  "The piano has 88 keys.",
  "Human ears detect 20Hz-20kHz frequencies.",
  "Guitar strings vibrate to create sound.",
  "Digital music began in 1980s."
};

// Select a random music fact
void selectRandomFact() {
  // Use ESP32's hardware random number generator
  int index = esp_random() % 10; // 10 is the size of our array
  aiSuggestion = musicFacts[index];
  
  Serial.println("\n==== AI SUGGESTION ====");
  Serial.println(aiSuggestion);
  Serial.println("=======================\n");
}

void setup() {
  // Increase ESP32 CPU frequency to maximum
  setCpuFrequencyMhz(240);
  
  // Set microSD Card CS pin
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  // Start serial communication
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 I2S Music Player Starting...");
  
  // Print initial heap size
  Serial.print("Initial free heap: ");
  Serial.println(ESP.getFreeHeap());

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed!");
  }

  // Initialize SPI for SD card
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  // Initialize SD card
  if (!SD.begin(SD_CS, SPI)) {
    Serial.println("SD card initialization failed!");
    while (true);
  }

  Serial.println("SD card initialized.");

  // Set up I2S for audio output
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

  // Set volume level (0–100)
  audio.setVolume(100);

  // Play the initial audio file
  audio.connecttoFS(SD, "/DEVICE-START-VOICE.mp3");

  // Select a random music fact instead of using the API
  selectRandomFact();
  Serial.println("Using locally stored music fact");
}

void loop() {
  // Handle audio processing
  audio.loop();
  
  // Small delay to prevent CPU hogging
  delay(10);
}

