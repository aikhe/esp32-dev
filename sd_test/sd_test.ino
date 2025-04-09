#include <SPI.h>
#include <SD.h>

#define CS_PIN 5

void setup() {
  Serial.begin(115200);

  if (!SD.begin(CS_PIN)) {
    Serial.println("Card Mount Failed");
    return;
  }

  Serial.println("SD Card initialized.");
}

void loop() {
}
s