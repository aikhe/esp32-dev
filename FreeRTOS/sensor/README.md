# ESP32 HC-SR04 Water Level Monitor with Audio Alerts

This project uses an ESP32 to monitor water levels using an HC-SR04 ultrasonic sensor and provides audio alerts via a PCM5102a I2S DAC when different water levels are detected.

## Hardware Requirements

- ESP32 Development Board
- HC-SR04 Ultrasonic Distance Sensor
- PCM5102a I2S DAC Audio Module
- microSD Card Module
- microSD Card (formatted as FAT32)
- 16x2 I2C LCD Display
- 3 LEDs (for visual alerts)
- Resistors for LEDs (220Ω recommended)
- Jumper wires
- Breadboard or PCB

## Pin Connections

### HC-SR04 Ultrasonic Sensor
- TRIG_PIN: GPIO 17
- ECHO_PIN: GPIO 16

### LEDs
- LED_ONE (Alert): GPIO 13
- LED_TWO (Critical): GPIO 12
- LED_THREE (Warning): GPIO 14

### microSD Card Module
- CS: GPIO 5
- MOSI: GPIO 23
- MISO: GPIO 19
- SCK: GPIO 18

### PCM5102a I2S DAC
- BCLK: GPIO 27
- LRC: GPIO 26
- DIN: GPIO 25

### I2C LCD Display
- SDA: GPIO 21
- SCL: GPIO 22

## Required Audio Files

The following audio files must be placed on the root directory of the microSD card:

1. `DEVICE-START-VOICE.mp3` - Played when the device starts up
2. `LOW-ALERT-HIGH.mp3` - Played when water level is between 25-40cm (Alert)
3. `MEDIUM-ALERT-HIGH.mp3` - Played when water level is between 15-25cm (Critical)
4. `HIGH-ALERT-HIGH.mp3` - Played when water level is between 0-15cm (Warning)

## Audio File Format

- Format: MP3
- Recommended bit rate: 128kbps or higher
- Recommended sample rate: 44.1kHz

## Setting Up the SD Card

1. Format the microSD card as FAT32
2. Copy the required audio files to the root directory of the SD card
3. Insert the SD card into the microSD card module

## Operation

The system monitors water levels using the HC-SR04 ultrasonic sensor and triggers different alerts based on the detected distance:

- **Alert (LED_ONE)**: Water level is between 25-40cm
- **Critical (LED_TWO)**: Water level is between 15-25cm
- **Warning (LED_THREE)**: Water level is between 0-15cm

When a new water level is detected, the system:
1. Turns on the corresponding LED
2. Updates the LCD display with the current distance and status
3. Plays the corresponding audio alert

The system includes a 5-second debounce to prevent rapid changes in alerts and a 10-minute timeout that turns off the LEDs and resets the display if no new water level is detected.

## Troubleshooting

If the system is not playing audio:

1. Check that all audio files are present on the SD card
2. Verify the SD card is properly formatted as FAT32
3. Check the connections between the ESP32, SD card module, and PCM5102a
4. Make sure the audio files are in MP3 format

If the system shows "SD Card Error" on the LCD:
1. Check that the SD card is properly inserted
2. Try reformatting the SD card
3. Verify the connections between the ESP32 and SD card module

## Dependencies

This project uses the following libraries:
- Audio.h (ESP32-audioI2S)
- LiquidCrystal_I2C
- Wire
- SPI
- SD
- FS

Make sure to install these libraries in your Arduino IDE before uploading the code. 