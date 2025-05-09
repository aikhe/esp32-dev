#ifndef SAMPLE_AUDIO_H
#define SAMPLE_AUDIO_H

#include <stdint.h>

// Simple 440Hz sine wave test tone (A4 note)
// 16-bit stereo samples at 44.1kHz
// This generates approximately 1 second of audio

const size_t audio_data_len = 44100 * 4; // 1 second of 16-bit stereo audio
uint8_t audio_data[audio_data_len];

// Function to generate a test tone
void generateTestTone() {
  // Generate a 440 Hz sine wave
  const float amplitude = 32767.0;  // Max amplitude for 16-bit
  const float frequency = 440.0;    // A4 note (440 Hz)
  const float period = 44100.0;     // Sample rate
  
  for (int i = 0; i < audio_data_len / 4; i++) {
    // Calculate the sine wave value
    float sinValue = sin(2 * PI * frequency * i / period);
    int16_t sampleValue = (int16_t)(sinValue * amplitude);
    
    // Store the same value in both left and right channels
    // Each sample takes 4 bytes (16-bit stereo)
    int idx = i * 4;
    
    // Left channel (little endian)
    audio_data[idx] = sampleValue & 0xFF;
    audio_data[idx + 1] = (sampleValue >> 8) & 0xFF;
    
    // Right channel (little endian)
    audio_data[idx + 2] = sampleValue & 0xFF;
    audio_data[idx + 3] = (sampleValue >> 8) & 0xFF;
  }
}

// For using your own audio data in WAV format:
// 1. Convert your WAV file to a C array using a tool like xxd or an online converter
// 2. Replace the test tone with your actual audio data
// 3. Make sure to update audio_data_len to match your file size

/*
// Example of how to include your own audio data:
const size_t audio_data_len = YOUR_FILE_SIZE;
const uint8_t audio_data[] = {
  0x52, 0x49, 0x46, 0x46, ... // Your raw audio data here
};
*/

#endif // SAMPLE_AUDIO_H
