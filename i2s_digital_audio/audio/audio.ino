#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"

// Replace with your WiFi credentials
const char* ssid = "TK-gacura";
const char* password = "gisaniel924";

// I2S pin configuration
#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC  26

// Create instances for streaming
URLStream urlStream(ssid, password);
I2SStream i2sStream;
EncodedAudioStream decoder(&i2sStream, new MP3DecoderHelix());
StreamCopy streamCopier(decoder, urlStream);

void setup() {
  Serial.begin(115200);
  AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Info);

  // Configure I2S with specified pins
  auto i2sConfig = i2sStream.defaultConfig(TX_MODE);
  i2sConfig.pin_bck = I2S_BCLK;
  i2sConfig.pin_ws = I2S_LRC;
  i2sConfig.pin_data = I2S_DOUT;
  i2sStream.begin(i2sConfig);

  // Initialize the decoder
  decoder.begin();

  // Start streaming from the URL
  urlStream.begin("http://stream.srg-ssr.ch/m/rsj/mp3_128", "audio/mp3");
}

void loop() {
  // Continuously copy data from the URL stream to the decoder
  streamCopier.copy();
}
