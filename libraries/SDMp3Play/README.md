# SDMp3Play

A simplified library for playing MP3 files from SD card on ESP32 microcontrollers.

## Description

SDMp3Play is a lightweight library based on the ESP32-audioI2S library that focuses specifically on MP3 playback from an SD card. It provides a simple API for playing MP3 files with basic controls like play, pause, and volume adjustment.

## Features

- Easy to use API for playing MP3 files from SD card
- Control playback (play, pause, stop)
- Adjust volume
- Get information about the playing audio file (duration, bitrate, sample rate)
- Simple and lightweight

## Installation

1. Download the library as a ZIP file
2. Open the Arduino IDE
3. Go to Sketch > Include Library > Add .ZIP Library...
4. Select the downloaded ZIP file

## Hardware Setup

This library requires an ESP32 connected to:
- An I2S DAC (like MAX98357A, PCM5102, etc.)
- An SD card reader

Typical connections for I2S:
- BCLK (Bit Clock) - GPIO pin
- LRC (Left/Right Clock or Word Select) - GPIO pin
- DOUT (Data Out) - GPIO pin
- MCLK (Master Clock) - Optional, connect to GPIO or leave unconnected

For SD card, use the standard SPI connections.

## Usage

```cpp
#include <Arduino.h>
#include <SD.h>
#include <SDMp3Play.h>

// Create an instance of the SDMp3Play class
SDMp3Play mp3Player;

void setup() {
  Serial.begin(115200);
  
  // Initialize SD card
  if (!SD.begin()) {
    Serial.println("SD card initialization failed!");
    return;
  }
  
  // Initialize the MP3 player
  if (!mp3Player.begin()) {
    Serial.println("MP3 player initialization failed!");
    return;
  }
  
  // Set I2S pins
  mp3Player.setPinout(25, 26, 22); // BCLK, LRC, DOUT
  
  // Set volume (0-100)
  mp3Player.setVolume(80);
  
  // Play MP3 file from SD card
  if (!mp3Player.connectToSD("/music.mp3")) {
    Serial.println("Failed to open MP3 file!");
    return;
  }
  
  Serial.println("Playing MP3 file from SD card");
}

void loop() {
  // Keep the MP3 decoder running
  mp3Player.loop();
  
  // Print information every second
  static unsigned long lastTime = 0;
  if (millis() - lastTime > 1000) {
    lastTime = millis();
    
    if (mp3Player.isRunning()) {
      Serial.print("Time: ");
      Serial.print(mp3Player.getCurrentTime());
      Serial.print(" / ");
      Serial.print(mp3Player.getDuration());
      Serial.print(" seconds, Bitrate: ");
      Serial.print(mp3Player.getBitRate());
      Serial.println(" kbps");
    }
  }
}
```

## API Reference

### Constructor

- `SDMp3Play(uint8_t i2sPort = I2S_NUM_0)` - Creates an instance of the SDMp3Play class

### Core Functions

- `bool begin()` - Initializes the MP3 decoder
- `bool connectToSD(const char* path)` - Opens and starts playing an MP3 file from SD card
- `bool isRunning()` - Returns true if the player is currently playing
- `void loop()` - Must be called regularly in the main loop to process audio data
- `uint32_t stop()` - Stops playback
- `bool pauseResume()` - Toggles between pause and resume

### Configuration

- `bool setPinout(uint8_t BCLK, uint8_t LRC, uint8_t DOUT, int8_t MCLK = I2S_GPIO_UNUSED)` - Sets the I2S pins
- `void setVolume(uint8_t vol)` - Sets the volume (0-100)
- `uint8_t getVolume()` - Gets the current volume

### Playback Information

- `uint32_t getFileSize()` - Returns the size of the MP3 file in bytes
- `uint32_t getFilePos()` - Returns the current position in the file
- `uint32_t getSampleRate()` - Returns the sample rate of the MP3
- `uint8_t getChannels()` - Returns the number of channels (1=mono, 2=stereo)
- `uint32_t getBitRate()` - Returns the bitrate of the MP3 in kbps
- `uint32_t getDuration()` - Returns the approximate duration in seconds
- `uint32_t getCurrentTime()` - Returns the approximate current playback time in seconds

## License

This library is based on the ESP32-audioI2S library and retains its original license.

## Credits

This library is a simplified version based on the ESP32-audioI2S library. 