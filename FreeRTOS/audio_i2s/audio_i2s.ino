/**
 * @file player-sd-i2s.ino
 * @brief example using the SD library
 * 
 * @author Phil Schatzmann
 * @copyright GPLv3
 */

#define AUDIO_USE_PSRAM 
#include "AudioTools.h"
#include "AudioTools/Disk/AudioSourceSD.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"

// I2S DAC pin configuration
#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC  26

const char* startFilePath = "/";
const char* ext = "mp3";
AudioSourceSD source(startFilePath, ext);
I2SStream i2s;
MP3DecoderHelix decoder;
AudioPlayer player(source, i2s, decoder);

void printMetaData(MetaDataType type, const char* str, int len) {
  Serial.print("==> ");
  Serial.print(toStr(type));
  Serial.print(": ");
  Serial.println(str);
}

void setup() {
  Serial.begin(115200);
  AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Info);

  // setup output
  auto cfg = i2s.defaultConfig(TX_MODE);
  cfg.pin_bck = I2S_BCLK;
  cfg.pin_ws = I2S_LRC;
  cfg.pin_data = I2S_DOUT;
  cfg.mode = I2S_STD_FORMAT;
  i2s.begin(cfg);

  // setup player
  //source.setFileFilter("*Bob Dylan*");
  player.setMetadataCallback(printMetaData);
  player.begin();
}

void loop() {
  player.copy();
}

