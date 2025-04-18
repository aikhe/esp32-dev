#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"

// I2S pins
#define I2S_DOUT  25
#define I2S_BCLK  27
#define I2S_LRC   26

Audio audio;

// WiFi creds
const char* ssid     = "TK-gacura";
const char* password = "gisaniel924";

// Sentences to speak
const char* floodSentences[] = {
  "Paalala sa lahat ng residente: May matinding banta ng pagbaha sa inyong lugar.",
  "Lumikas agad patungo sa mas mataas na lugar.",
  "Dalhin ang mahahalagang gamit at manatiling kalmado.",
  "Makinig sa mga anunsyo ng lokal na pamahalaan para sa karagdagang impormasyon."
};
const uint8_t numSentences = sizeof(floodSentences) / sizeof(floodSentences[0]);

void setup() {
  Serial.begin(115200);

  // connect WiFi
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  // setup I2S + volume
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(100);

  // kick it off immediately
  playFloodWarning();
}

void loop() {
  // stream audio
  audio.loop();

  // as soon as everything’s done, play again
  if (!audio.isRunning()) {
    delay(300);               // tiny pause so Google doesn’t get hammered
    playFloodWarning();
  }
}

void playFloodWarning() {
  for (uint8_t i = 0; i < numSentences; i++) {
    audio.connecttospeech(floodSentences[i], "tl");
    while (audio.isRunning()) {
      audio.loop();
    }
    delay(0);  // gap between sentences
  }
}
