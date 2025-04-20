/*
  ESP32 SD I2S Music Player
  Plays MP3 file from microSD card
  Uses MAX98357 I2S Amplifier Module
  Uses ESP32-audioI2S Library - https://github.com/schreibfaul1/ESP32-audioI2S
  
  DroneBot Workshop 2022
  https://dronebotworkshop.com
*/

// Include required libraries
#include "Arduino.h"
#include "Audio.h"
#include "SD.h"
#include "FS.h"
#include "SPI.h"

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

void setup() {
  // Set microSD Card CS as OUTPUT and set HIGH
  pinMode(SD_CS, OUTPUT);      
  digitalWrite(SD_CS, HIGH); 

  // Start Serial Port
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 I2S Music Player Starting...");

  // Initialize SPI bus for microSD Card
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  // Initialize microSD card with custom SPI
  if (!SD.begin(SD_CS, SPI)) {
    Serial.println("Error accessing microSD card!");
    while (true); 
  }

  Serial.println("microSD card initialized.");

  // Setup I2S 
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

  // Set Volume (0 to 21)
  audio.setVolume(100);

  // Attempt to play MP3 file
  if (!audio.connecttoFS(SD, "crow.mp3")) {
    Serial.println("Failed to connect to /crow.mp3");
  } else {
    Serial.println("Playing /crow.mp3");
  }
}

void loop() {
  audio.loop();

  // If playback has ended, restart the MP3
  if (!audio.isRunning()) {
    Serial.println("Restarting /crow.mp3");
    audio.connecttoFS(SD, "MEDIUM-FLOOD.mp3");
  }
}
