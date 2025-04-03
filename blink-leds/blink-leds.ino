/*
  Blink

  Turns an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO
  it is attached to digital pin 13, on MKR1000 on pin 6.  is set to
  the correct LED pin independent of which board is used.
  If you want to know what pin the on-board LED is connected to on your Arduino
  model, check the Technical Specs of your board at:
  https://www.arduino.cc/en/Main/Products

  modified 8 May 2014
  by Scott Fitzgerald
  modified 2 Sep 2016
  by Arturo Guadalupi
  modified 8 Sep 2016
  by Colby Newman

  This example code is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/Blink
*/

// Define LED pins
int LED_ONE = 3;
int LED_TWO = 1;
int LED_THREE = 22;

void setup() {
  // Set LEDs as outputs
  pinMode(LED_ONE, OUTPUT);
  pinMode(LED_TWO, OUTPUT);
  pinMode(LED_THREE, OUTPUT);
}

void loop() {
  // First LED ON
  digitalWrite(LED_ONE, HIGH);
  delay(200);
  
  // Second LED ON, First LED OFF
  digitalWrite(LED_ONE, LOW);
  digitalWrite(LED_TWO, HIGH);
  delay(200);

  // Third LED ON, Second LED OFF
  digitalWrite(LED_TWO, LOW);
  digitalWrite(LED_THREE, HIGH);
  delay(200);

  // Third LED OFF to restart the cycle
  digitalWrite(LED_THREE, LOW);
}
