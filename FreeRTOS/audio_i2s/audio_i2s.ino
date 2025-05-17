#include <SPI.h>
#include <SD.h>
#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"

// SD card SPI configuration
#define SD_CS     5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK  18

// I2S DAC pin configuration
#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC  26

// Create instances for audio components
I2SStream i2sStream;
MP3DecoderHelix mp3Decoder;
EncodedAudioStream decoder(&i2sStream, &mp3Decoder);
File mp3File;
StreamCopy copier;

void setup() {
  Serial.begin(115200);
  AudioLogger::instance().begin(Serial, AudioLogger::Info);

  // Initialize SPI bus
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);

  // Initialize SD card
  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed!");
    while (true);
  }

  // Open MP3 file from SD card
  mp3File = SD.open("/DEVICE-START-VOICE.mp3");
  if (!mp3File) {
    Serial.println("Failed to open MP3 file!");
    while (true);
  }

  // Configure I2S with specified pins
  auto i2sConfig = i2sStream.defaultConfig(TX_MODE);
  i2sConfig.pin_bck = I2S_BCLK;
  i2sConfig.pin_ws = I2S_LRC;
  i2sConfig.pin_data = I2S_DOUT;
  i2sConfig.buffer_size = 8192;  // Increase buffer size
  i2sConfig.buffer_count = 8;    // Increase buffer count
  i2sConfig.channels        = 2;
  i2sConfig.channel_format  = I2SChannelSelect::Stereo;
  i2sStream.begin(i2sConfig);

  // Initialize decoder
  decoder.begin();

  // Set up copier
  copier.begin(decoder, mp3File);
}

void loop() {
  // Continuously copy data from the MP3 file to the decoder
  copier.copy();
}
