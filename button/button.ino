int button = 3;
int buttonState = 0;

void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:
  pinMode(button, INPUT_PULLUP);
}

void loop() {
  // put your main code here, to run repeatedly:
  buttonState = digitalRead(button);

  Serial.println(buttonState);
  delay(100);
}
