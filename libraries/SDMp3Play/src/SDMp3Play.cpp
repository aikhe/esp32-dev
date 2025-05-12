/*
 * SDMp3Play.cpp
 * A simplified library for playing MP3 files from SD card on ESP32
 * Based on ESP32-audioI2S library
 */

#include "SDMp3Play.h"
#include "mp3_decoder.h"

// MP3 decoder variables (will be initialized in the MP3 decoder)
extern MP3FrameInfo_t  MP3FrameInfo;
extern int16_t         m_outBuf[2304]; // MP3 output buffer, enough for up to 1152 samples * 2 channels
static uint8_t         m_i2s_num;

SDMp3Play::SDMp3Play(uint8_t i2sPort) {
    m_i2s_num = i2sPort;
    
    // Set default values
    setDefaults();
}

SDMp3Play::~SDMp3Play() {
    stop();
    if(m_inBuff) free(m_inBuff);
    if(m_outBuff) free(m_outBuff);
}

void SDMp3Play::setDefaults() {
    // Initialize all member variables
    m_f_running = false;
    m_f_paused = false;
    m_fileSize = 0;
    m_volume = 64; // Default volume (0-100)
    m_sampleRate = 44100;
    m_channels = 2;
    m_bitRate = 0;
    
    // Default I2S pins
    m_BCLK = 0;
    m_LRC = 0;
    m_DOUT = 0;
    m_MCLK = I2S_GPIO_UNUSED;
    
    // Create buffers
    m_inBuffSize = 2048; // Input buffer size
    m_outBuffSize = 2048; // Output buffer size
    
    if(m_inBuff) free(m_inBuff);
    if(m_outBuff) free(m_outBuff);
    
    m_inBuff = (uint8_t*)malloc(m_inBuffSize);
    m_outBuff = (uint8_t*)malloc(m_outBuffSize);
    
    if(!m_inBuff || !m_outBuff) {
        log_e("Out of memory for buffers");
    }
}

bool SDMp3Play::begin() {
    // Allocate buffers for the MP3 decoder
    if(!MP3Decoder_AllocateBuffers()) {
        log_e("Failed to allocate MP3 decoder buffers");
        return false;
    }
    return true;
}

bool SDMp3Play::setPinout(uint8_t BCLK, uint8_t LRC, uint8_t DOUT, int8_t MCLK) {
    m_BCLK = BCLK;
    m_LRC = LRC;
    m_DOUT = DOUT;
    m_MCLK = MCLK;
    
    // I2S configuration
    i2s_config_t i2s_config;
    memset(&i2s_config, 0, sizeof(i2s_config));
    i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    i2s_config.sample_rate = m_sampleRate;
    i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    i2s_config.dma_buf_count = 8;
    i2s_config.dma_buf_len = 64;
    i2s_config.use_apll = false;
    i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    
    // Install and start I2S driver
    esp_err_t err = i2s_driver_install((i2s_port_t)m_i2s_num, &i2s_config, 0, NULL);
    if(err != ESP_OK) {
        log_e("I2S driver installation failed: %d", err);
        return false;
    }
    
    // I2S pin configuration
    i2s_pin_config_t pin_config;
    pin_config.bck_io_num = m_BCLK;
    pin_config.ws_io_num = m_LRC;
    pin_config.data_out_num = m_DOUT;
    pin_config.data_in_num = I2S_PIN_NO_CHANGE;
    
    // Try to set the pins without MCLK if it's set to I2S_GPIO_UNUSED
    err = i2s_set_pin((i2s_port_t)m_i2s_num, &pin_config);
    if(err != ESP_OK) {
        log_e("I2S pin configuration failed: %d", err);
        return false;
    }
    
    return true;
}

bool SDMp3Play::connectToSD(const char* path) {
    if(!path) {
        log_e("Path is null");
        return false;
    }
    
    // Check if file exists and has .mp3 extension
    if(!SD.exists(path) || !endsWith(path, ".mp3")) {
        log_e("File not found or not an MP3: %s", path);
        return false;
    }
    
    log_i("Opening file: %s", path);
    
    // Stop any current playback
    stop();
    
    // Open the file
    audiofile = SD.open(path);
    if(!audiofile) {
        log_e("Failed to open file: %s", path);
        return false;
    }
    
    m_fileSize = audiofile.size();
    
    // Initialize MP3 decoder
    if(!initializeDecoder()) {
        log_e("Failed to initialize MP3 decoder");
        audiofile.close();
        return false;
    }
    
    m_f_running = true;
    m_f_paused = false;
    
    return true;
}

bool SDMp3Play::initializeDecoder() {
    // Read first chunk of data to identify the MP3 stream
    size_t bytesRead = audiofile.read(m_inBuff, m_inBuffSize);
    if(bytesRead == 0) {
        log_e("Failed to read from file");
        return false;
    }
    
    // Find the sync word in the MP3 stream
    int offset = MP3FindSyncWord(m_inBuff, bytesRead);
    if(offset < 0) {
        log_e("MP3 sync word not found");
        return false;
    }
    
    // Get MP3 info from frame header
    if(MP3GetNextFrameInfo(m_inBuff + offset) < 0) {
        log_e("Failed to get MP3 frame info");
        return false;
    }
    
    // Get MP3 properties
    m_sampleRate = MP3GetSampRate();
    m_channels = MP3GetChannels();
    m_bitRate = MP3GetBitrate();
    
    // Reset file position to the beginning
    audiofile.seek(0);
    
    return true;
}

bool SDMp3Play::isRunning() {
    return m_f_running;
}

void SDMp3Play::loop() {
    if(!m_f_running || m_f_paused) return;
    
    processLocalFile();
}

void SDMp3Play::processLocalFile() {
    size_t bytesLeft = 0;
    
    // Read data from file
    size_t bytesRead = audiofile.read(m_inBuff, m_inBuffSize);
    if(bytesRead == 0) {
        // End of file
        log_i("End of file reached");
        stop();
        return;
    }
    
    bytesLeft = bytesRead;
    uint8_t* readPtr = m_inBuff;
    
    // Find the sync word in the buffer
    int offset = MP3FindSyncWord(readPtr, bytesLeft);
    if(offset < 0) {
        // No sync word found, try with more data
        return;
    }
    
    readPtr += offset;
    bytesLeft -= offset;
    
    // Decode MP3 data
    int bytesDecoded = MP3Decode(readPtr, (int32_t*)&bytesLeft, m_outBuf, 0);
    if(bytesDecoded < 0) {
        // Error decoding MP3 frame
        log_e("MP3 decoding error: %d", bytesDecoded);
        return;
    }
    
    // Update file position
    int bytesConsumed = bytesRead - bytesLeft;
    audiofile.seek(audiofile.position() - bytesLeft);
    
    // Get frame info
    int numSamples = MP3GetOutputSamps();
    
    // Play audio through I2S
    playChunk();
}

void SDMp3Play::playChunk() {
    if(!m_f_running || m_f_paused) return;
    
    // Get number of samples in the decoded frame
    int numSamples = MP3GetOutputSamps();
    if(numSamples <= 0) return;
    
    // Apply volume (simple gain control)
    int16_t* samples = m_outBuf;
    for(int i = 0; i < numSamples * MP3GetChannels(); i++) {
        samples[i] = (int16_t)((samples[i] * m_volume) / 100);
    }
    
    // Write to I2S
    size_t bytesWritten = 0;
    i2s_write((i2s_port_t)m_i2s_num, samples, numSamples * MP3GetChannels() * 2, &bytesWritten, portMAX_DELAY);
}

uint32_t SDMp3Play::stop() {
    if(!m_f_running) return 0;
    
    m_f_running = false;
    m_f_paused = false;
    
    // Close file
    if(audiofile) audiofile.close();
    
    // Stop I2S
    stopI2S();
    
    // Clear MP3 decoder buffer
    MP3Decoder_ClearBuffer();
    
    return 1;
}

bool SDMp3Play::pauseResume() {
    if(!m_f_running) return false;
    
    m_f_paused = !m_f_paused;
    return true;
}

bool SDMp3Play::startI2S() {
    esp_err_t err = i2s_start((i2s_port_t)m_i2s_num);
    return (err == ESP_OK);
}

bool SDMp3Play::stopI2S() {
    esp_err_t err = i2s_stop((i2s_port_t)m_i2s_num);
    return (err == ESP_OK);
}

void SDMp3Play::setVolume(uint8_t vol) {
    if(vol > 100) vol = 100;
    m_volume = vol;
}

uint8_t SDMp3Play::getVolume() {
    return m_volume;
}

uint32_t SDMp3Play::getFileSize() {
    return m_fileSize;
}

uint32_t SDMp3Play::getFilePos() {
    return audiofile ? audiofile.position() : 0;
}

uint32_t SDMp3Play::getSampleRate() {
    return m_sampleRate;
}

uint8_t SDMp3Play::getChannels() {
    return m_channels;
}

uint32_t SDMp3Play::getBitRate() {
    return m_bitRate;
}

uint32_t SDMp3Play::getDuration() {
    // Calculate approximate duration in seconds
    if(m_bitRate > 0 && m_fileSize > 0) {
        return m_fileSize * 8 / (m_bitRate * 1000);
    }
    return 0;
}

uint32_t SDMp3Play::getCurrentTime() {
    // Calculate approximate current time in seconds
    if(m_bitRate > 0 && audiofile) {
        return audiofile.position() * 8 / (m_bitRate * 1000);
    }
    return 0;
}

// Helper function to check if a string ends with a specific suffix
bool endsWith(const char *base, const char *searchString) {
    int baseLen = strlen(base);
    int searchLen = strlen(searchString);
    if(searchLen > baseLen) return false;
    return (strcasecmp(base + baseLen - searchLen, searchString) == 0);
} 