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

// Task handle for audio playback
TaskHandle_t audioTaskHandle;

// Audio playback task
void audioTask(void *parameter) {
  while (true) {
    audio.loop();

    if (!audio.isRunning()) {
      Serial.println("Restarting audio...");
      audio.connecttoFS(SD, "/DEVICE-START-VOICE.mp3");
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void setup() {
  // Set microSD Card CS pin
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  // Start serial communication
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 I2S Music Player Starting (FreeRTOS)...");

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

  // Create FreeRTOS task for audio playback
  xTaskCreate(
    audioTask,           // Task function
    "AudioTask",         // Name
    4096,                // Stack size
    NULL,                // Parameter
    1,                   // Priority
    &audioTaskHandle     // Task handle
  );
}

void loop() {
  // Main loop does nothing—audio handled in task
}
