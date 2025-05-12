/*
 * SDMp3Play.h
 * A simplified library for playing MP3 files from SD card on ESP32
 * Based on ESP32-audioI2S library
 */

#pragma once
#include <Arduino.h>
#include <SD.h>
#include <FS.h>
#include <driver/i2s.h>

#ifndef I2S_GPIO_UNUSED
  #define I2S_GPIO_UNUSED -1 // = I2S_PIN_NO_CHANGE in IDF < 5
#endif

// Helper function to check if a string ends with a specific suffix
bool endsWith(const char *base, const char *searchString);

class SDMp3Play {
public:
    SDMp3Play(uint8_t i2sPort = I2S_NUM_0);
    ~SDMp3Play();
    
    // Core functions
    bool begin();
    bool connectToSD(const char* path);
    bool isRunning();
    void loop();
    uint32_t stop();
    bool pauseResume();
    
    // Configuration
    bool setPinout(uint8_t BCLK, uint8_t LRC, uint8_t DOUT, int8_t MCLK = I2S_GPIO_UNUSED);
    void setVolume(uint8_t vol);
    uint8_t getVolume();
    
    // Playback info
    uint32_t getFileSize();
    uint32_t getFilePos();
    uint32_t getSampleRate();
    uint8_t getChannels();
    uint32_t getBitRate();
    uint32_t getDuration();
    uint32_t getCurrentTime();
    
private:
    void setDefaults();
    bool initializeDecoder();
    void processLocalFile();
    void playChunk();
    bool startI2S();
    bool stopI2S();
    
    // File and buffer handling
    File audiofile;
    uint8_t* m_outBuff;
    uint8_t* m_inBuff;
    uint32_t m_fileSize;
    
    // I2S configuration
    uint8_t m_i2s_num;
    uint8_t m_BCLK;
    uint8_t m_LRC;
    uint8_t m_DOUT;
    int8_t m_MCLK;
    
    // Audio state
    bool m_f_running;
    bool m_f_paused;
    uint8_t m_volume;
    uint32_t m_sampleRate;
    uint8_t m_channels;
    uint32_t m_bitRate;
    
    // Buffer sizes
    size_t m_inBuffSize;
    size_t m_outBuffSize;
}; 