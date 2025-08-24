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
static bool s_open = false;

esp_err_t audio_alert_init(void)
{
    if (s_ready) return ESP_OK;
    s_spk = bsp_audio_codec_speaker_init();
    if (!s_spk) {
        ESP_LOGE(TAG, "speaker init failed");
        return ESP_FAIL;
    }
    // Avoid disabling I2S channels on every esp_codec_dev_close(),
    // which may log errors if a channel wasn't enabled by the codec layer.
    // We keep I2S managed by BSP and just close the codec path cleanly.
    (void)esp_codec_set_disable_when_closed(s_spk, false);
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
    if (!s_open) {
        if (esp_codec_dev_open(s_spk, &fs) != ESP_OK) return;
        s_open = true;
    }
    // Give codec/PA a short settle time before streaming to avoid pops
    vTaskDelay(pdMS_TO_TICKS(20));
    // Ensure unmuted for playback
    (void)esp_codec_dev_set_out_mute(s_spk, false);

    // Write a short block of silence first to avoid initial click/pop
    enum { ZERO_PAD_SAMP = 1024 }; // ~46ms at 22.05kHz
    int16_t zero_pad[ZERO_PAD_SAMP] = {0};
    (void)esp_codec_dev_write(s_spk, (void*)zero_pad, sizeof(zero_pad));

    // Stream in small chunks to ensure DMA feed
    const size_t chunk_samp = 256;
    size_t written = 0;
    while (written < samples) {
        size_t n = samples - written;
        if (n > chunk_samp) n = chunk_samp;
        (void)esp_codec_dev_write(s_spk, (void*)(pcm + written), n * sizeof(int16_t));
        written += n;
    }

    // Add a short tail of silence to ensure clean ramp-down
    (void)esp_codec_dev_write(s_spk, (void*)zero_pad, sizeof(zero_pad));

    // Wait for audio to drain before closing (approximate duration)
    uint32_t total_samples = samples + (2 * ZERO_PAD_SAMP);
    uint32_t ms = (uint32_t)((total_samples * 1000UL) / (fs.sample_rate));
    vTaskDelay(pdMS_TO_TICKS(ms + 10));
    // Optionally mute between alerts to avoid residual noise; keep stream open
    (void)esp_codec_dev_set_out_mute(s_spk, true);
}

void audio_alert_notify(void)
{
    if (!settings_get_sound()) return;
    // Generate a short "dong" with exponential decay at ~660 Hz
    enum { SR = 22050 };
    const float freq = 660.0f;
    const float dur_s = 0.20f;
    size_t N = (size_t)(SR * dur_s);
    static int16_t buf[8192];
    size_t max_samples = sizeof(buf) / sizeof(buf[0]);
    if (N > max_samples) {
        // Cap length to buffer instead of early return
        N = max_samples;
    }
    // Apply short fade-in to remove the initial click/pop
    const float attack_s = 0.006f; // ~6ms
    const size_t attack_n = (size_t)(attack_s * SR);
    for (size_t n = 0; n < N; ++n) {
        float t = (float)n / (float)SR;
        float env = expf(-8.0f * t); // fast decay
        float s = sinf(2.0f * 3.14159265f * freq * t) * env;
        // Linear attack ramp
        float fade_in = 1.0f;
        if (n < attack_n) {
            fade_in = (float)n / (float)(attack_n);
        }
        s *= fade_in;
        int v = (int)(s * 32000.0f); // near full-scale without clipping
        if (v > 32767) v = 32767; else if (v < -32768) v = -32768;
        buf[n] = (int16_t)v;
    }
    play_pcm_16_mono_22k(buf, N);
}

// Delayed startup tone to avoid first-play click during boot
static void audio_startup_tone_task(void *pv)
{
    (void)pv;
    // Allow system/codec to fully settle
    vTaskDelay(pdMS_TO_TICKS(400));
    audio_alert_notify();
    vTaskDelete(NULL);
}

void audio_alert_play_startup(void)
{
    if (!settings_get_sound()) return;
    // Create a detached task to play startup tone after a brief delay
    xTaskCreate(audio_startup_tone_task, "tone_startup", 8192, NULL, 3, NULL);
}
