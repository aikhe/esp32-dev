#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

// microSD Card Reader connections
#define SD_CS 5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18

// I2S Connections (PCM5102A)
#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC 26

// File name of MP3 to play
#define MP3_FILENAME "/DEVICE-START-VOICE.mp3"

// Audio objects
AudioGeneratorMP3 *mp3 = NULL;
AudioFileSourceSD *file = NULL;
AudioOutputI2S *out = NULL;

// Performance tuning variables
unsigned long lastPlayTime = 0;
const unsigned long LOOP_DELAY = 10; // Minimum delay between loop calls

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  
  // Give some time for serial to initialize
  delay(1000);
  Serial.println("MP3 Player Initializing...");

  // Configure I2S pins before SPI
  pinMode(I2S_BCLK, OUTPUT);
  pinMode(I2S_LRC, OUTPUT);
  pinMode(I2S_DOUT, OUTPUT);

  // Initialize SPI for SD Card with optimized settings
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  SPI.setFrequency(16000000); // Increase SPI clock to 16 MHz
  
  // Initialize SD Card with multiple attempts
  int sdInitAttempts = 0;
  while (!SD.begin(SD_CS) && sdInitAttempts < 3) {
    Serial.print("SD Card mount failed. Attempt ");
    Serial.print(sdInitAttempts + 1);
    Serial.println(" of 3");
    delay(500);
    sdInitAttempts++;
  }
  
  if (sdInitAttempts == 3) {
    Serial.println("Failed to mount SD Card after 3 attempts");
    while (1);
  }
  Serial.println("SD Card mounted successfully");

  // Check if MP3 file exists
  if (!SD.exists(MP3_FILENAME)) {
    Serial.println("MP3 file not found!");
    Serial.print("Checked filename: ");
    Serial.println(MP3_FILENAME);
    
    // List files in root directory for debugging
    File root = SD.open("/");
    Serial.println("Files in root directory:");
    while (true) {
      File entry = root.openNextFile();
      if (!entry) {
        // No more files
        break;
      }
      Serial.print("  ");
      Serial.println(entry.name());
      entry.close();
    }
    root.close();
    
    while (1);
  }

  // Create audio output object with optimized settings
  out = new AudioOutputI2S(0, AudioOutputI2S::EXTERNAL_I2S);
  
  // Configure I2S output with specific settings for ESP32
  out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  
  // Adjust gain carefully
  out->SetGain(1.0); // Full gain, adjust if too loud

  // Open the file
  file = new AudioFileSourceSD(MP3_FILENAME);
  
  // Create MP3 decoder
  mp3 = new AudioGeneratorMP3();

  // Start playing with error checking
  if (mp3->begin(file, out)) {
    Serial.println("MP3 playback started");
    
    // Print file size for debugging
    Serial.print("File size: ");
    Serial.print(file->getSize());
    Serial.println(" bytes");
  } else {
    Serial.println("MP3 playback failed to start");
    while(1);
  }
}

void loop() {
  // Ensure we don't call loop too frequently
  unsigned long currentTime = millis();
  
  // Critical: Ensure continuous playback
  if (mp3->isRunning()) {
    // Call loop with minimal delay to prevent audio interruption
    if (!mp3->loop()) {
      // Playback finished
      Serial.println("MP3 playback finished");
      
      // Stop and clean up
      mp3->stop();
      delete mp3;
      delete file;
      delete out;
      
      // Restart or halt
      while(1);
    }
  } else {
    Serial.println("MP3 playback stopped unexpectedly");
    while(1);
  }
  
  // Small delay to prevent overwhelming the processor
  delay(LOOP_DELAY);
}
