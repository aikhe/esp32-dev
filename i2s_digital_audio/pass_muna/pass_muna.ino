#include <driver/i2s.h>

#define I2S_NUM         I2S_NUM_0
#define I2S_BCLK        26
#define I2S_LRCLK       25
#define I2S_DOUT        22

void setup() {
  Serial.begin(115200);

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRCLK,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM, &pin_config);
}

void loop() {
  const int samples = 256;
  int16_t buffer[samples];
  float freq = 440.0; // A4 note
  for (int i = 0; i < samples; i++) {
    float sample = sin(2.0 * PI * freq * i / 44100);
    buffer[i] = (int16_t)(sample * 32767);
  }

  size_t bytes_written;
  i2s_write(I2S_NUM, buffer, sizeof(buffer), &bytes_written, portMAX_DELAY);
}
