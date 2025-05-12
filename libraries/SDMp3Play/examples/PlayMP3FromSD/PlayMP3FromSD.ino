/*
 * PlayMP3FromSD.ino
 * Example for the SDMp3Play library
 * 
 * This example shows how to use the SDMp3Play library to play MP3 files from an SD card.
 * 
 * Hardware:
 * - ESP32 Dev Module
 * - SD card reader connected to ESP32 via SPI
 * - I2S amplifier or DAC like MAX98357A, PCM5102, etc.
 * 
 * Connections:
 * SD Card:
 * - MOSI: GPIO 23
 * - MISO: GPIO 19
 * - SCK: GPIO 18
 * - CS: GPIO 5
 * 
 * I2S Audio:
 * - BCLK: GPIO 25
 * - LRC: GPIO 26
 * - DOUT: GPIO 22
 * 
 * Controls:
 * - GPIO 12: Play/Pause button (connect to GND to trigger)
 * - GPIO 13: Stop button (connect to GND to trigger)
 * - GPIO 14: Next file button (connect to GND to trigger)
 */

#include <Arduino.h>
#include <SD.h>
#include <SDMp3Play.h>

// Pin definitions
// SD Card
#define SD_CS          5

// I2S pins
#define I2S_BCLK      25
#define I2S_LRC       26
#define I2S_DOUT      22

// Control buttons
#define BTN_PLAY_PAUSE 12
#define BTN_STOP       13
#define BTN_NEXT       14

// Create an instance of the SDMp3Play class
SDMp3Play mp3Player;

// Variables for MP3 files
String mp3Files[20];  // Array to store MP3 filenames (max 20)
int fileCount = 0;    // Number of MP3 files found
int currentFile = 0;  // Index of current playing file

// Button state tracking
bool lastPlayPauseState = HIGH;
bool lastStopState = HIGH;
bool lastNextState = HIGH;

void setup() {
  Serial.begin(115200);
  Serial.println("SDMp3Play Example - Play MP3 from SD card");
  
  // Setup button pins
  pinMode(BTN_PLAY_PAUSE, INPUT_PULLUP);
  pinMode(BTN_STOP, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);

  // Initialize SD card
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed!");
    while (1);  // Stop here
  }
  Serial.println("SD card initialized.");
  
  // Scan for MP3 files
  scanForMP3Files();
  
  if (fileCount == 0) {
    Serial.println("No MP3 files found on SD card!");
    while (1);  // Stop here
  }
  
  // Initialize the MP3 player
  if (!mp3Player.begin()) {
    Serial.println("MP3 player initialization failed!");
    while (1);  // Stop here
  }
  
  // Set I2S pins
  mp3Player.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  
  // Set volume (0-100)
  mp3Player.setVolume(80);
  
  // Play first MP3 file
  playFile(0);
}

void loop() {
  // Keep the MP3 decoder running
  mp3Player.loop();
  
  // Check for button presses
  handleButtons();
  
  // Display playback information every second
  static unsigned long lastInfoTime = 0;
  if (millis() - lastInfoTime > 1000) {
    lastInfoTime = millis();
    
    if (mp3Player.isRunning()) {
      // Print playback information
      Serial.print("Playing: ");
      Serial.print(mp3Files[currentFile]);
      Serial.print(" (");
      Serial.print(currentFile + 1);
      Serial.print(" of ");
      Serial.print(fileCount);
      Serial.println(")");
      
      Serial.print("Time: ");
      Serial.print(mp3Player.getCurrentTime());
      Serial.print(" / ");
      Serial.print(mp3Player.getDuration());
      Serial.print(" seconds, Bitrate: ");
      Serial.print(mp3Player.getBitRate());
      Serial.println(" kbps");
      
      Serial.print("Sample Rate: ");
      Serial.print(mp3Player.getSampleRate());
      Serial.print(" Hz, Channels: ");
      Serial.println(mp3Player.getChannels());
      
      Serial.println();
    }
  }
}

void handleButtons() {
  // Play/Pause button
  bool currentPlayPauseState = digitalRead(BTN_PLAY_PAUSE);
  if (currentPlayPauseState == LOW && lastPlayPauseState == HIGH) {
    mp3Player.pauseResume();
    Serial.println(mp3Player.isRunning() ? "Resumed" : "Paused");
    delay(50);  // Debounce
  }
  lastPlayPauseState = currentPlayPauseState;
  
  // Stop button
  bool currentStopState = digitalRead(BTN_STOP);
  if (currentStopState == LOW && lastStopState == HIGH) {
    mp3Player.stop();
    Serial.println("Stopped");
    delay(50);  // Debounce
  }
  lastStopState = currentStopState;
  
  // Next file button
  bool currentNextState = digitalRead(BTN_NEXT);
  if (currentNextState == LOW && lastNextState == HIGH) {
    // Play next file
    currentFile = (currentFile + 1) % fileCount;
    playFile(currentFile);
    delay(50);  // Debounce
  }
  lastNextState = currentNextState;
}

void scanForMP3Files() {
  Serial.println("Scanning for MP3 files...");
  
  fileCount = 0;
  File root = SD.open("/");
  scanDirectory(root, 0);
  root.close();
  
  Serial.print("Found ");
  Serial.print(fileCount);
  Serial.println(" MP3 files:");
  
  for (int i = 0; i < fileCount; i++) {
    Serial.print(i+1);
    Serial.print(". ");
    Serial.println(mp3Files[i]);
  }
  Serial.println();
}

void scanDirectory(File dir, int numTabs) {
  while (true) {
    File entry = dir.openNextFile();
    
    if (!entry) {
      // No more files
      break;
    }
    
    if (entry.isDirectory()) {
      // Skip directories
    } else {
      String filename = entry.name();
      if (filename.endsWith(".mp3") || filename.endsWith(".MP3")) {
        if (fileCount < 20) {  // Maximum of 20 files
          mp3Files[fileCount] = "/" + filename;
          fileCount++;
        }
      }
    }
    
    entry.close();
  }
}

void playFile(int index) {
  if (index < 0 || index >= fileCount) return;
  
  Serial.print("Playing file: ");
  Serial.println(mp3Files[index]);
  
  if (!mp3Player.connectToSD(mp3Files[index].c_str())) {
    Serial.println("Failed to open MP3 file!");
    return;
  }
  
  // Store current file index
  currentFile = index;
} 