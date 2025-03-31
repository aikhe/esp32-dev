int ledPin = 2;
String cmd;

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing...");

  pinMode(ledPin, OUTPUT);
}

void loop() {
  if (Serial.available()) {
    cmd = Serial.readString();

    if (cmd == "on") {
      digitalWrite(ledPin, HIGH);
      Serial.println("LED On");
    } else if (cmd == "off") {
      digitalWrite(ledPin, LOW);
      Serial.println("LED Off");
    }
  } else {
    digitalWrite(ledPin, HIGH);
    delay(500);
    
    digitalWrite(ledPin, LOW);
    delay(500);
  }
}
