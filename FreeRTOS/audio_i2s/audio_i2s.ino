/*
 * ESP32 Button MP3 Player
 * When a button is pressed, plays MP3 audio from SD card via MAX98357 I2S DAC
 * Using FreeRTOS for task management
 */

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <SD.h>
#include <SPI.h>
#include "Audio.h"

// Pin Definitions
#define BUTTON_PIN 4      // Button pin
#define DEBOUNCE_TIME 50  // Debounce time in milliseconds

// microSD Card Reader connections
#define SD_CS 5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18

// I2S Connections (MAX98357)
#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC 26

// Audio object for ESP32-audioI2S library
Audio audio;

// Queue handle for communication between tasks
QueueHandle_t buttonQueue;

// Variable to track button state
volatile bool buttonPressed = false;
volatile uint32_t lastDebounceTime = 0;

// Task handles
TaskHandle_t buttonTaskHandle = NULL;
TaskHandle_t audioTaskHandle = NULL;

// Button monitoring task
void buttonTask(void *parameter) {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  bool buttonState;
  bool lastButtonState = HIGH;
  
  while(1) {
    buttonState = digitalRead(BUTTON_PIN);
    
    // Check if button state changed
    if (buttonState != lastButtonState) {
      // Debounce
      if ((millis() - lastDebounceTime) > DEBOUNCE_TIME) {
        lastDebounceTime = millis();
        
        // If button is pressed (LOW due to pull-up)
        if (buttonState == LOW) {
          int pressEvent = 1;
          xQueueSend(buttonQueue, &pressEvent, 0);
        }
        
        lastButtonState = buttonState;
      }
    }
    
    vTaskDelay(10 / portTICK_PERIOD_MS);  // Small delay to prevent CPU hogging
  }
}

// Audio task - handles playing audio
void audioTask(void *parameter) {
  int event;
  
  while(1) {
    // Wait for event from button task
    if (xQueueReceive(buttonQueue, &event, portMAX_DELAY)) {
      if (event == 1) {
        Serial.println("Button pressed! Playing audio file...");
        
        // Stop any currently playing audio
        audio.stopSong();
        
        // Play MP3 file from SD card
        audio.connecttoFS(SD, "/DEVICE-START-VOICE.mp3");
        
        // Wait until audio is finished
        while (audio.isRunning()) {
          audio.loop();
          vTaskDelay(1 / portTICK_PERIOD_MS);
        }
        
        Serial.println("Audio playback complete");
      }
    }
    
    // No delay needed as task blocks on queue
  }
}

// Initialize SD card
bool initSDCard() {
  // Configure SPI pins for SD card
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  
  if (!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    return false;
  }
  
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return false;
  }

  Serial.println("SD Card Initialized");
  
  // Check if our audio file exists
  if (!SD.exists("/DEVICE-START-VOICE.mp3")) {
    Serial.println("Cannot find NEW-NUM-REG-HIGH.mp3 file");
    return false;
  }
  
  Serial.println("Found MP3 file");
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Button Audio Player");
  
  // Initialize SD card
  if (!initSDCard()) {
    Serial.println("SD card initialization failed!");
    while(1); // Stop execution if SD card initialization fails
  }
  
  // Set up I2S configuration for MAX98357
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(100); // 0-21
  
  // Create queue for button events
  buttonQueue = xQueueCreate(10, sizeof(int));
  
  if (buttonQueue == NULL) {
    Serial.println("Error creating the queue");
    return;
  }
  
  // Create tasks
  xTaskCreate(
    buttonTask,         // Task function
    "ButtonTask",       // Name of task
    2048,               // Stack size
    NULL,               // Parameter to pass
    1,                  // Priority
    &buttonTaskHandle   // Task handle
  );
  
  xTaskCreate(
    audioTask,          // Task function
    "AudioTask",        // Name of task
    4096,               // Stack size (increased for audio processing)
    NULL,               // Parameter to pass
    1,                  // Priority
    &audioTaskHandle    // Task handle
  );
}

void loop() {
  // Empty as FreeRTOS tasks handle everything
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
