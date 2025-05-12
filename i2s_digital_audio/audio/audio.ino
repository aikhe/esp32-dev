#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include <SPI.h>
#include <SD.h>

#define SD_CS    5

SPIClass spiSD(VSPI);
AudioFileSourceSD source("/DEVICE-START-VOICE.mp3");
AudioGeneratorMP3 mp3;
AudioOutputI2S out;

void setup(){
  Serial.begin(115200);
  spiSD.begin(18, 19, 23, SD_CS);
  SD.begin(SD_CS, spiSD);

  out.begin();                 // uses GPIO 26,25,22 by default
  source.begin();
  mp3.begin(&source, &out);
}

void loop(){
  if(mp3.isRunning()) mp3.loop();
}
