int LED_ONE = 21;
int LED_TWO = 22;
int LED_THREE = 23;

// Define button pin
int buttonPin = 3;
int buttonState = 0;
int lastButtonState = HIGH;  // Variable to track the previous button state
bool isRunning = false;  // Variable to track LED state

// LED timing variables
unsigned long previousMillis = 0;
unsigned long interval = 300;  // 300ms interval for LEDs
int currentLED = 0;  // Keep track of which LED to turn on

void setup() {
  Serial.begin(115200);

  pinMode(LED_ONE, OUTPUT);
  pinMode(LED_TWO, OUTPUT);
  pinMode(LED_THREE, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);  // Enable pull-up resistor
}

void loop() {
  int reading = digitalRead(buttonPin);

  // If the button state has changed (pressed or released)
  if (reading != lastButtonState) {
    previousMillis = millis();  // Reset debounce timer
  }

  if ((millis() - previousMillis) > 50) {  // Debounce time (50ms)
    if (reading != buttonState) {
      buttonState = reading;

      // Only toggle the state if the button is pressed
      if (buttonState == LOW) {  // Button is pressed
        isRunning = !isRunning;
        Serial.print("isRunning: ");
        Serial.println(isRunning);
      }
    }
  }

  lastButtonState = reading;  // Save the current button state for the next loop

  // If isRunning is true, run the LED wave pattern
  if (isRunning) {
    unsigned long currentMillis = millis();  // Get the current time

    // Check if it's time to switch to the next LED
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;  // Save the last time an LED was toggled

      // Turn off the current LED
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

      // Move to the next LED
      currentLED = (currentLED + 1) % 3;  // Loop between 0, 1, 2
    }
  } else {
    // Turn off all LEDs if isRunning is false
    digitalWrite(LED_ONE, LOW);
    digitalWrite(LED_TWO, LOW);
    digitalWrite(LED_THREE, LOW);
  }
}
