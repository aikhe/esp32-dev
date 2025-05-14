#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

#define SD_CS     5
#define SPI_MOSI  23
#define SPI_MISO  19
#define SPI_SCK   18

#define I2S_DOUT  25
#define I2S_BCLK  27
#define I2S_LRC   26

#define MP3_FILENAME "/DEVICE-START-VOICE.mp3"

AudioGeneratorMP3 *mp3;
AudioFileSourceSD  *file;
AudioFileSourceID3 *id3;
AudioOutputI2S     *out;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Initializing SD card...");

  // Initialize SD card on VSPI
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPIClass SDSPI(VSPI);
  SDSPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  if (!SD.begin(SD_CS, SDSPI)) {
    Serial.println("ERROR: SD card init failed!");
    while (1);  // halt
  }
  // Check if MP3 file exists
  if (!SD.exists(MP3_FILENAME)) {
    Serial.println("ERROR: MP3 file not found!");
    while (1);
  }
  Serial.println("SD card initialized.");

  // Set up audio objects
  file = new AudioFileSourceSD(MP3_FILENAME);
  id3  = new AudioFileSourceID3(file);
  out  = new AudioOutputI2S();
  out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  out->SetGain(1.0);  // set volume gain (1.0 = no attenuation)

  mp3 = new AudioGeneratorMP3();
  mp3->begin(id3, out);
  Serial.println("MP3 playback started.");
}

void loop() {
  if (mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
      Serial.println("MP3 playback finished.");
    }
  }
  delay(1);
}
