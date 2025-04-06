#include <Arduino.h>

#define TRIG_PIN 12
#define ECHO_PIN 13

long duration;

void setup() {
  Serial.begin(115200);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  duration = pulseIn(ECHO_PIN, HIGH);
  
  Serial.print("Raw duration: ");
  Serial.print(duration);
  Serial.print(" μs");

  Serial.print(" Distance: ");
  Serial.print(duration / 58.773);
  Serial.println(" cm");
  
  delay(1000);
}
