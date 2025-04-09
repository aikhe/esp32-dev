#include "Arduino.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "driver/i2s.h"

// MicroSD Pin definitions
#define SD_CS          5
#define SPI_MOSI      23
#define SPI_MISO      19
#define SPI_SCK       18

// I2S Pin definitions
#define I2S_DOUT      22
#define I2S_BCLK      26
#define I2S_LRC       25

// Initialize I2S
void initI2S() {
    // Define the I2S configuration
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),  // Correct usage of I2S_MODE
        .sample_rate = 44100,                                  // 44.1kHz sample rate
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,           // 16-bit sample size
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,           // Stereo output
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,               // Interrupt level
        .dma_buf_count = 8,                                     // DMA buffer count
        .dma_buf_len = 1024                                    // DMA buffer length
    };
    
    // Pin configuration for I2S
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRC,
        .data_out_num = I2S_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    // Initialize I2S driver
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
}

void setup() {
    // Initialize SD card
    if (!SD.begin(SD_CS)) {
        Serial.println("Error accessing SD card!");
        while (true);
    }
    
    // Initialize I2S
    initI2S();

    // Open MP3 file (note: we're not processing MP3 here, just a placeholder for I2S stream)
    File audioFile = SD.open("/crow.mp3");
    if (!audioFile) {
        Serial.println("Failed to open MP3 file!");
        while (true);
    }

    // Now start reading audio data and sending it to I2S (this part depends on your audio file format)
    // The logic to decode MP3 to PCM needs to be handled here
    // For simplicity, let's assume it's already PCM data in the file
    while (audioFile.available()) {
        uint8_t data[1024]; // Adjust based on buffer size
        int bytesRead = audioFile.read(data, sizeof(data));
        i2s_write(I2S_NUM_0, data, bytesRead, NULL, portMAX_DELAY);  // Use i2s_write instead of i2s_write_bytes
    }
    
    // Close the file when done playing
    audioFile.close();
    
    // Reset file pointer to start for looping
    audioFile = SD.open("/crow.mp3");
}

void loop() {
    // Loop the audio
    File audioFile = SD.open("/crow.mp3");
    if (!audioFile) {
        Serial.println("Failed to open MP3 file!");
        while (true);
    }

    // Play the audio in a loop
    while (audioFile.available()) {
        uint8_t data[1024]; // Adjust based on buffer size
        int bytesRead = audioFile.read(data, sizeof(data));
        i2s_write(I2S_NUM_0, data, bytesRead, NULL, portMAX_DELAY);  // Write data to I2S for playback
    }

    // When the file reaches the end, reset the file pointer
    audioFile.seek(0); // Reset the file pointer to loop the audio
}
