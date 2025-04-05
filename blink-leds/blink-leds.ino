int LED_ONE = 21;
int LED_TWO = 22;
int LED_THREE = 23;

int buttonPin = 3;
int buttonState = 0;
int lastButtonState = HIGH;

bool isRunning = false;

unsigned long previousMillis = 0;
unsigned long interval = 300;
int currentLED = 0;

void setup() {
  Serial.begin(115200);

  pinMode(LED_ONE, OUTPUT);
  pinMode(LED_TWO, OUTPUT);
  pinMode(LED_THREE, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
}

void loop() {
  int reading = digitalRead(buttonPin);

  // toggle leds (pressed or released)
  if (reading != lastButtonState) {
    previousMillis = millis();  // Reset debounce timer (only need once to toggle running state and set the value by currentMillis)
  }

  if ((millis() - previousMillis) > 50) {  // Debounce time (50ms) for consistent toggling, always run once button was clicked
    if (reading != buttonState) {
      buttonState = reading;
      Serial.print("buttonState: ");
      Serial.println(buttonState);

      // Only toggle leds state if the button is pressed (0/LOW)
      if (buttonState == LOW) {
        isRunning = !isRunning;
        Serial.print("isRunning: ");
        Serial.println(isRunning);
      }
    }
  }

  lastButtonState = reading;  // Reset button state to 1/HIGH

  if (isRunning) {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis; // update previousMills state

      if (currentLED == 0) {
        digitalWrite(LED_ONE, HIGH);
        digitalWrite(LED_TWO, LOW);
        digitalWrite(LED_THREE, LOW);
      } else if (currentLED == 1) {
        digitalWrite(LED_ONE, LOW);
        digitalWrite(LED_TWO, HIGH);
        digitalWrite(LED_THREE, LOW);
      } else if (currentLED == 2) {
        digitalWrite(LED_ONE, LOW);
        digitalWrite(LED_TWO, LOW);
        digitalWrite(LED_THREE, HIGH);
      }

      currentLED = (currentLED + 1) % 3;  // Loop between 0, 1, 2 (gets the median)
    }
  } else {
    digitalWrite(LED_ONE, LOW);
    digitalWrite(LED_TWO, LOW);
    digitalWrite(LED_THREE, LOW);
  }
}
