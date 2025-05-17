/**
 * Simple LED Animation Task using FreeRTOS for ESP32 with Arduino IDE
 */

// LED pin definitions
#define AI_LED_ONE 32
#define AI_LED_TWO 15
#define AI_LED_THREE 33

// Task handle
TaskHandle_t ledTaskHandle = NULL;

/**
 * LED Animation Task
 */
void ledAnimationTask(void *parameter) {
  // Define sequence
  int leds[] = { AI_LED_ONE, AI_LED_TWO, AI_LED_THREE };
  int sequence[] = { 0, 1, 2, 0, 1, 2, -1 };  // -1 = all LEDs on
  int currentStep = 0;
  
  // Run LED sequence while audio is playing or animation not complete
  while (currentStep < 7) {
    // Turn off all LEDs before applying the next step
    for (int j = 0; j < 3; j++) {
      digitalWrite(leds[j], LOW);
    }
    
    if (currentStep < 7) {
      if (sequence[currentStep] == -1) {
        // Turn all LEDs ON
        vTaskDelay(pdMS_TO_TICKS(200));
        for (int j = 0; j < 3; j++) {
          digitalWrite(leds[j], HIGH);
        }
      } else {
        digitalWrite(leds[sequence[currentStep]], HIGH);
      }
      currentStep++;
    }
    
    vTaskDelay(pdMS_TO_TICKS(200));
  }
  
  // Turn off all LEDs when done
  for (int i = 0; i < 3; i++) {
    digitalWrite(leds[i], LOW);
  }
  
  // Delete the task
  vTaskDelete(NULL);
}

void setup() {
  // Configure LED pins
  pinMode(AI_LED_ONE, OUTPUT);
  pinMode(AI_LED_TWO, OUTPUT);
  pinMode(AI_LED_THREE, OUTPUT);
  
  // Create the LED animation task
  xTaskCreate(
    ledAnimationTask,  // Task function
    "LEDTask",         // Task name
    2000,              // Stack size
    NULL,              // Task parameters
    1,                 // Priority
    &ledTaskHandle     // Task handle
  );
}

void loop() {
  // Your main code here
  delay(100);
}
