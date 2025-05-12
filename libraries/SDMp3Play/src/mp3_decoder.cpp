/*
 * mp3_decoder.cpp - Simplified MP3 decoder based on Helix MP3 decoder
 * For use with the SDMp3Play library
 */

#include "mp3_decoder.h"

// MP3 decoder buffers and state
static MP3DecInfo_t* hMP3Decoder = NULL;
static SideInfo_t*   hSideInfo = NULL;
static ScaleFactorInfoSub_t* hScaleFactors = NULL;
static HuffmanInfo_t* hHuffmanInfo = NULL;
static IMDCTInfo_t*   hIMDCTInfo = NULL;
static SubbandInfo_t* hSubbandInfo = NULL;

// Tables for MP3 decoding - will be defined in the library that includes this file
extern const SFBandTable_t sfBandTable[3][3];
extern const int samplerateTab[3][3];
extern const short bitrateTab[3][3][15];

// Global variables
MP3FrameInfo_t MP3FrameInfo;
int16_t m_outBuf[2304];

// Implementation of public functions

bool MP3Decoder_AllocateBuffers(void) {
    // Allocate all the decoder buffers at once
    hMP3Decoder = (MP3DecInfo_t*)calloc(1, sizeof(MP3DecInfo_t));
    hSideInfo = (SideInfo_t*)calloc(1, sizeof(SideInfo_t));
    hScaleFactors = (ScaleFactorInfoSub_t*)calloc(m_MAX_NCHAN, sizeof(ScaleFactorInfoSub_t));
    hHuffmanInfo = (HuffmanInfo_t*)calloc(1, sizeof(HuffmanInfo_t));
    hIMDCTInfo = (IMDCTInfo_t*)calloc(1, sizeof(IMDCTInfo_t));
    hSubbandInfo = (SubbandInfo_t*)calloc(1, sizeof(SubbandInfo_t));
    
    if (!hMP3Decoder || !hSideInfo || !hScaleFactors || !hHuffmanInfo || !hIMDCTInfo || !hSubbandInfo) {
        // Free any allocated buffers on failure
        MP3Decoder_FreeBuffers();
        return false;
    }
    
    // Initialize the decoder info
    hMP3Decoder->nChans = 2;  // Default to stereo
    hMP3Decoder->samprate = 44100;  // Default to 44.1kHz
    
    return true;
}

bool MP3Decoder_IsInit() {
    return (hMP3Decoder != NULL);
}

void MP3Decoder_FreeBuffers() {
    if (hMP3Decoder) free(hMP3Decoder);
    if (hSideInfo) free(hSideInfo);
    if (hScaleFactors) free(hScaleFactors);
    if (hHuffmanInfo) free(hHuffmanInfo);
    if (hIMDCTInfo) free(hIMDCTInfo);
    if (hSubbandInfo) free(hSubbandInfo);
    
    hMP3Decoder = NULL;
    hSideInfo = NULL;
    hScaleFactors = NULL;
    hHuffmanInfo = NULL;
    hIMDCTInfo = NULL;
    hSubbandInfo = NULL;
}

void MP3Decoder_ClearBuffer(void) {
    if (hMP3Decoder) {
        memset(hMP3Decoder->mainBuf, 0, m_MAINBUF_SIZE);
    }
}

// Helper function to find the synchronization word in an MP3 frame
int32_t MP3FindSyncWord(uint8_t *buf, int32_t nBytes) {
    int32_t i;
    
    // A valid MPEG sync word has 11 consecutive 1's
    for (i = 0; i < nBytes - 1; i++) {
        if ((buf[i] == 0xFF) && ((buf[i+1] & 0xE0) == 0xE0))
            return i;
    }
    
    return -1;
}

// Stub for MP3 decode - in a real implementation this would decode the MP3 frame
int32_t MP3Decode(uint8_t *inbuf, int32_t *bytesLeft, int16_t *outbuf, int32_t useSize) {
    if (!inbuf || !bytesLeft || !outbuf || !hMP3Decoder)
        return ERR_MP3_NULL_POINTER;
    
    // Find the sync word
    int offset = MP3FindSyncWord(inbuf, *bytesLeft);
    if (offset < 0)
        return ERR_MP3_INDATA_UNDERFLOW;
    
    // Extract header information
    uint8_t *frameHeader = inbuf + offset;
    
    // Parse header (simplified)
    int mpegVersion = (frameHeader[1] >> 3) & 0x03;
    int layer = (frameHeader[1] >> 1) & 0x03;
    int bitrateIndex = (frameHeader[2] >> 4) & 0x0F;
    int samplerateIndex = (frameHeader[2] >> 2) & 0x03;
    int channelMode = (frameHeader[3] >> 6) & 0x03;
    
    // Convert to actual values
    int bitrate = 0;
    if (layer == 1) { // Layer III
        bitrate = bitrateTab[mpegVersion][2][bitrateIndex]; // in kbps
    }
    
    int samplerate = samplerateTab[mpegVersion][samplerateIndex];
    int channels = (channelMode == 3) ? 1 : 2; // Mono or Stereo
    
    // Update frame info
    MP3FrameInfo.bitrate = bitrate;
    MP3FrameInfo.nChans = channels;
    MP3FrameInfo.samprate = samplerate;
    MP3FrameInfo.bitsPerSample = 16; // Always 16 bits per sample for MP3
    MP3FrameInfo.outputSamps = 1152; // For MPEG1, Layer III
    MP3FrameInfo.layer = 4 - layer;
    MP3FrameInfo.version = mpegVersion;
    
    // Update decoder info
    hMP3Decoder->bitrate = bitrate;
    hMP3Decoder->nChans = channels;
    hMP3Decoder->samprate = samplerate;
    
    // In a real decoder, we would decode the actual audio data here
    // For this simplified version, we'll simulate decoding by generating silence
    
    // Generate silence
    memset(outbuf, 0, MP3FrameInfo.outputSamps * channels * sizeof(int16_t));
    
    // Adjust bytesLeft (this would be a proper frame size calculation in a real decoder)
    int frameSize = (144 * bitrate * 1000) / samplerate;
    if (mpegVersion == 0) frameSize += (frameSize % 4); // padding for MPEG1
    
    *bytesLeft -= frameSize;
    
    return 0; // Success
}

// Get information from the last decoded frame
void MP3GetLastFrameInfo() {
    // This function would typically copy the decoder's frame info to a public structure
    // Our simplified version stores it directly in MP3FrameInfo
}

// Get information about the next MP3 frame
int32_t MP3GetNextFrameInfo(uint8_t *buf) {
    if (!buf || !hMP3Decoder)
        return ERR_MP3_NULL_POINTER;
    
    int offset = MP3FindSyncWord(buf, 32); // Look for sync in first 32 bytes
    if (offset < 0)
        return ERR_MP3_INDATA_UNDERFLOW;
    
    // Parse header (simplified)
    uint8_t *frameHeader = buf + offset;
    
    int mpegVersion = (frameHeader[1] >> 3) & 0x03;
    int layer = (frameHeader[1] >> 1) & 0x03;
    int bitrateIndex = (frameHeader[2] >> 4) & 0x0F;
    int samplerateIndex = (frameHeader[2] >> 2) & 0x03;
    int channelMode = (frameHeader[3] >> 6) & 0x03;
    
    // Convert to actual values
    int bitrate = 0;
    if (layer == 1) { // Layer III
        bitrate = bitrateTab[mpegVersion][2][bitrateIndex]; // in kbps
    }
    
    int samplerate = samplerateTab[mpegVersion][samplerateIndex];
    int channels = (channelMode == 3) ? 1 : 2; // Mono or Stereo
    
    // Update frame info
    MP3FrameInfo.bitrate = bitrate;
    MP3FrameInfo.nChans = channels;
    MP3FrameInfo.samprate = samplerate;
    MP3FrameInfo.bitsPerSample = 16; // Always 16 bits per sample for MP3
    MP3FrameInfo.outputSamps = 1152; // For MPEG1, Layer III
    MP3FrameInfo.layer = 4 - layer;
    MP3FrameInfo.version = mpegVersion;
    
    return 0; // Success
}

// Common accessor functions
int32_t MP3GetSampRate() {
    return MP3FrameInfo.samprate;
}

int32_t MP3GetChannels() {
    return MP3FrameInfo.nChans;
}

int32_t MP3GetBitsPerSample() {
    return MP3FrameInfo.bitsPerSample;
}

int32_t MP3GetBitrate() {
    return MP3FrameInfo.bitrate;
}

int32_t MP3GetOutputSamps() {
    return MP3FrameInfo.outputSamps;
}

int32_t MP3GetLayer() {
    return MP3FrameInfo.layer;
}

int32_t MP3GetVersion() {
    return MP3FrameInfo.version;
}

// MP3 decoding tables
const int samplerateTab[3][3] = {
    {44100, 48000, 32000},  // MPEG 1
    {22050, 24000, 16000},  // MPEG 2
    {11025, 12000, 8000}    // MPEG 2.5
};

const short bitrateTab[3][3][15] = {
    {   // MPEG 1
        {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448}, // Layer 1
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},    // Layer 2
        {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}      // Layer 3
    },
    {   // MPEG 2
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},    // Layer 1
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},         // Layer 2
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}          // Layer 3
    },
    {   // MPEG 2.5
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},    // Layer 1
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},         // Layer 2
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}          // Layer 3
    }
};

// Simplified scale factor band table
const SFBandTable_t sfBandTable[3][3] = {
    {   // MPEG 1
        { {0, 4, 8, 12, 16, 20, 24, 30, 36, 44, 52, 62, 74, 90, 110, 134, 162, 196, 238, 288, 342, 418, 576}, {0, 4, 8, 12, 16, 22, 30, 40, 52, 66, 84, 106, 136, 192} }, // 44.1 kHz
        { {0, 4, 8, 12, 16, 20, 24, 30, 36, 42, 50, 60, 72, 88, 106, 128, 156, 190, 230, 276, 330, 384, 576}, {0, 4, 8, 12, 16, 22, 28, 38, 50, 64, 80, 100, 126, 192} }, // 48 kHz
        { {0, 4, 8, 12, 16, 20, 24, 30, 36, 44, 54, 66, 82, 102, 126, 156, 194, 240, 296, 364, 448, 550, 576}, {0, 4, 8, 12, 16, 22, 30, 42, 58, 78, 104, 138, 180, 192} }  // 32 kHz
    },
    {   // MPEG 2
        { {0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238, 284, 336, 396, 464, 522, 576}, {0, 4, 8, 12, 18, 24, 32, 42, 56, 74, 100, 132, 174, 192} }, // 22.05 kHz
        { {0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 114, 136, 162, 194, 232, 278, 330, 394, 464, 540, 576}, {0, 4, 8, 12, 18, 26, 36, 48, 62, 80, 104, 136, 180, 192} }, // 24 kHz
        { {0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238, 284, 336, 396, 464, 522, 576}, {0, 4, 8, 12, 18, 26, 36, 48, 62, 80, 104, 134, 174, 192} }  // 16 kHz
    },
    {   // MPEG 2.5
        { {0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238, 284, 336, 396, 464, 522, 576}, {0, 4, 8, 12, 18, 26, 36, 48, 62, 80, 104, 134, 174, 192} }, // 11.025 kHz
        { {0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238, 284, 336, 396, 464, 522, 576}, {0, 4, 8, 12, 18, 26, 36, 48, 62, 80, 104, 134, 174, 192} }, // 12 kHz
        { {0, 12, 24, 36, 48, 60, 72, 88, 108, 132, 160, 192, 232, 280, 336, 400, 476, 566, 568, 570, 572, 574, 576}, {0, 8, 16, 24, 36, 52, 72, 96, 124, 160, 162, 164, 166, 192} }  // 8 kHz
    },
}; 