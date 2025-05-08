/**
 * ESP32 HC-SR04 Distance Sensor with FreeRTOS
 *
 * This program reads distance from HC-SR04 ultrasonic sensor
 * and controls 3 LEDs based on the measured distance using FreeRTOS.
 */

// HC-SR04 Sensor pins
#define TRIG_PIN 17
#define ECHO_PIN 16

// LED pins
#define LED_ONE 13
#define LED_TWO 12
#define LED_THREE 14

// Constants
#define SOUND_SPEED 0.034  // Sound speed in cm/uS
#define DISTANCE_READ_INTERVAL 100  // ms
#define LED_UPDATE_INTERVAL 50      // ms

// Global variables for sharing data between tasks
volatile float currentDistance = 0;
SemaphoreHandle_t distanceMutex;

// Task function prototypes
void readDistanceTask(void *parameter);
void controlLEDsTask(void *parameter);

void setup() {
  // Initialize Serial communication
  Serial.begin(115200);
 
  // Configure HC-SR04 pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
 
  // Configure LED pins
  pinMode(LED_ONE, OUTPUT);
  pinMode(LED_TWO, OUTPUT);
  pinMode(LED_THREE, OUTPUT);
 
  // Initially turn off all LEDs
  digitalWrite(LED_ONE, LOW);
  digitalWrite(LED_TWO, LOW);
  digitalWrite(LED_THREE, LOW);
 
  // Create mutex for shared data protection
  distanceMutex = xSemaphoreCreateMutex();
 
  // Create FreeRTOS tasks
  xTaskCreate(
    readDistanceTask,     // Task function
    "ReadDistance",       // Task name
    2048,                 // Stack size (bytes)
    NULL,                 // Task parameters
    1,                    // Priority (1 is low)
    NULL                  // Task handle
  );
 
  xTaskCreate(
    controlLEDsTask,      // Task function
    "ControlLEDs",        // Task name
    2048,                 // Stack size (bytes)
    NULL,                 // Task parameters
    1,                    // Priority
    NULL                  // Task handle
  );
 
  Serial.println("ESP32 HC-SR04 Distance Sensor with FreeRTOS Started");
}

void loop() {
  // Empty loop as tasks are handling everything
  vTaskDelay(1000 / portTICK_PERIOD_MS); // Just keep the loop alive
}

/**
 * Task to read distance from HC-SR04 sensor
 */
void readDistanceTask(void *parameter) {
  float distance;
 
  while(true) {
    // Clears the TRIG_PIN
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    
    // Sets the TRIG_PIN HIGH for 10 microseconds
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    // Reads the ECHO_PIN, returns the sound wave travel time in microseconds
    float duration = pulseIn(ECHO_PIN, HIGH);
    
    // Calculate the distance
    distance = duration * SOUND_SPEED / 2;
    
    // Print the distance on the Serial Monitor
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
    
    // Update the shared distance variable with mutex protection
    if (xSemaphoreTake(distanceMutex, portMAX_DELAY) == pdTRUE) {
      currentDistance = distance;
      xSemaphoreGive(distanceMutex);
    }
    
    // Wait before next reading
    vTaskDelay(DISTANCE_READ_INTERVAL / portTICK_PERIOD_MS);
  }
}

/**
 * Task to control LEDs based on the measured distance
 */
void controlLEDsTask(void *parameter) {
  float distance;
  int currentLedState = 0;
  int targetLedState = 0;
 
  while(true) {
    // Get the current distance with mutex protection
    if (xSemaphoreTake(distanceMutex, portMAX_DELAY) == pdTRUE) {
      distance = currentDistance;
      xSemaphoreGive(distanceMutex);
    }
    
    // Determine target LED state based on current distance
    if (distance <= 10) {
      targetLedState = 3;  // LED_THREE
    } else if (distance <= 20) {
      targetLedState = 2;  // LED_TWO
    } else if (distance <= 40) {
      targetLedState = 1;  // LED_ONE
    } else {
      targetLedState = 0;  // All LEDs off
    }
    
    // Update LEDs if state has changed
    if (currentLedState != targetLedState) {
      // Turn all LEDs off first
      digitalWrite(LED_ONE, LOW);
      digitalWrite(LED_TWO, LOW);
      digitalWrite(LED_THREE, LOW);
      
      // Turn on only the appropriate LED based on targetLedState
      switch(targetLedState) {
        case 1:
          digitalWrite(LED_ONE, HIGH);
          break;
        case 2:
          digitalWrite(LED_TWO, HIGH);
          break;
        case 3:
          digitalWrite(LED_THREE, HIGH);
          break;
        // case 0: all LEDs are already off
      }
      
      currentLedState = targetLedState;
      
      // Log LED state change
      Serial.print("LED State: ");
      Serial.println(currentLedState);
    }
    
    // Wait before next update
    vTaskDelay(LED_UPDATE_INTERVAL / portTICK_PERIOD_MS);
  }
}
