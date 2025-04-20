#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"

#define I2S_DOUT  25
#define I2S_BCLK  27
#define I2S_LRC   26

#define TTS_GOOGLE_LANGUAGE "en"     // Use "tl" for Tagalog

Audio audio;

const char* ssid     = "TK-gacura";
const char* password = "gisaniel924";

// ✅ Dynamic weather update message
String floodMessage = 
  "AISuggestion: PRAF Technology Weather Update: Caloocan City is not expected to flood today; "
  "however, with scattered clouds, a temperature of 34.77°C but feeling like 41.77°C, and 58% humidity, "
  "remember to stay hydrated and avoid prolonged exposure to the sun.";

// 🔊 Speak in smart chunks
void speakTextInChunks(String text, int maxLength) {
  // Use a smaller chunk size
  int chunkSize = 60; // Reduced from 100
  
  int start = 0;
  while (start < text.length()) {
    int end = start + chunkSize;
    
    // Ensure we don't split in the middle of a word
    if (end < text.length()) {
      // Prefer ending at punctuation
      int punctEnd = end;
      while (punctEnd > start && text[punctEnd] != '.' && text[punctEnd] != ',' && text[punctEnd] != ';' && text[punctEnd] != ':') {
        punctEnd--;
      }
      
      // If we found punctuation, use that as the end point
      if (punctEnd > start && (text[punctEnd] == ',' || text[punctEnd] == ';' || text[punctEnd] == ':')) {
        end = punctEnd + 1; // Include the punctuation
      } else {
        // Otherwise find a space
        while (end > start && text[end] != ' ') {
          end--;
        }
        if (end == start) {
          end = start + chunkSize; // Worst case, just cut at max length
        }
      }
    }
    
    String chunk = text.substring(start, end);
    chunk.trim(); // Remove any leading/trailing spaces
    
    if (chunk.length() > 0) {
      Serial.println("Playing chunk: '" + chunk + "'");
      Serial.println("Start: " + String(start) + ", End: " + String(end));
      
      audio.connecttospeech(chunk.c_str(), TTS_GOOGLE_LANGUAGE);
      while (audio.isRunning()) {
        audio.loop();
      }
    }
    
    start = end;
  }
}

void playFloodWarning() {
  speakTextInChunks(floodMessage, 100); // Split into chunks of ~100 characters
}

// 🕒 Loop control
unsigned long lastPlayTime = 0;
const unsigned long playInterval = 60000UL;  // 1 minute

void setup() {
  Serial.begin(115200);

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(100);

  playFloodWarning();             // 🔊 Play once at start
  lastPlayTime = millis();
}

void loop() {
  audio.loop();

  // replay after interval
  if (!audio.isRunning() && (millis() - lastPlayTime >= playInterval)) {
    playFloodWarning();
    lastPlayTime = millis();
  }
}
