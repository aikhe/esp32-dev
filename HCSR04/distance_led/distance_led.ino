#define TRIG_PIN 18
#define ECHO_PIN 19

#define LED_ONE 17
#define LED_TWO 16
#define LED_THREE 4

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(LED_ONE, OUTPUT);
  pinMode(LED_TWO, OUTPUT);
  pinMode(LED_THREE, OUTPUT);
}

void loop() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH);
  
  float distance = duration / 58.773;
  
  Serial.print("Raw duration: ");
  Serial.print(duration);
  Serial.print(" μs");
  Serial.print(" Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  
  if (distance <= 10) {
    digitalWrite(LED_THREE, HIGH);
    digitalWrite(LED_ONE, LOW);
    digitalWrite(LED_TWO, LOW);
  } 
  else if (distance <= 20) {
    digitalWrite(LED_TWO, HIGH);
    digitalWrite(LED_ONE, LOW);
    digitalWrite(LED_THREE, LOW);
  } 
  else if (distance <= 30) {
    digitalWrite(LED_ONE, HIGH);
    digitalWrite(LED_TWO, LOW);
    digitalWrite(LED_THREE, LOW);
  }
  
  delay(1000);
}
