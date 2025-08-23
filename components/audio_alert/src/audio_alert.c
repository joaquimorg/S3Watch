#include "audio_alert.h"
#include <math.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "bsp/esp32_s3_touch_amoled_2_06.h"
#include "esp_codec_dev.h"
#include "settings.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "AUDIO_ALERT";

static esp_codec_dev_handle_t s_spk = NULL;
static bool s_ready = false;

esp_err_t audio_alert_init(void)
{
    if (s_ready) return ESP_OK;
    s_spk = bsp_audio_codec_speaker_init();
    if (!s_spk) {
        ESP_LOGE(TAG, "speaker init failed");
        return ESP_FAIL;
    }
    s_ready = true;
    return ESP_OK;
}

static void play_pcm_16_mono_22k(const int16_t* pcm, size_t samples)
{
    if (!s_ready && audio_alert_init() != ESP_OK) return;
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = 22050,
        .channel = 1,
        .bits_per_sample = 16,
    };
    int vol = (int)settings_get_notify_volume();
    if (vol < 0) vol = 0;
    if (vol > 100) vol = 100;
    esp_codec_dev_set_out_vol(s_spk, vol);
    if (esp_codec_dev_open(s_spk, &fs) != ESP_OK) return;
    // Give codec/PA a short settle time before streaming to avoid pops
    vTaskDelay(pdMS_TO_TICKS(5));
    // Stream in small chunks to ensure DMA feed
    const size_t chunk_samp = 256;
    size_t written = 0;
    while (written < samples) {
        size_t n = samples - written;
        if (n > chunk_samp) n = chunk_samp;
        (void)esp_codec_dev_write(s_spk, (void*)(pcm + written), n * sizeof(int16_t));
        written += n;
    }
    // Wait for audio to drain before closing (bytes / (bytes_per_ms))
    uint32_t ms = (uint32_t)((samples * 1000UL) / (fs.sample_rate));
    vTaskDelay(pdMS_TO_TICKS(ms + 10));
    esp_codec_dev_close(s_spk);
}

void audio_alert_notify(void)
{
    if (!settings_get_sound()) return;
    // Generate a short "dong" with exponential decay at ~660 Hz
    enum { SR = 22050 };
    const float freq = 660.0f;
    const float dur_s = 0.18f;
    const size_t N = (size_t)(SR * dur_s);
    static int16_t buf[4096];
    if (N > sizeof(buf)/sizeof(buf[0])) {
        // Cap length to buffer
        return;
    }
    for (size_t n = 0; n < N; ++n) {
        float t = (float)n / (float)SR;
        float env = expf(-8.0f * t); // fast decay
        float s = sinf(2.0f * 3.14159265f * freq * t) * env;
        int v = (int)(s * 30000.0f);
        if (v > 32767) v = 32767; else if (v < -32768) v = -32768;
        buf[n] = (int16_t)v;
    }
    play_pcm_16_mono_22k(buf, N);
}
