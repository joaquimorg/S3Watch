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
#include <stdio.h>
#include <sys/stat.h>

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

static bool play_pcm_stream_i16(const int16_t* pcm, size_t samples, int sample_rate, int channels)
{
    if (!s_ready && audio_alert_init() != ESP_OK) return false;
    if (channels != 1 && channels != 2) channels = 1;
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = sample_rate,
        .channel = channels,
        .bits_per_sample = 16,
    };
    int vol = (int)settings_get_notify_volume();
    if (vol < 0) vol = 0;
    if (vol > 100) vol = 100;
    esp_codec_dev_set_out_vol(s_spk, vol);

    if (s_open) {
        (void)esp_codec_dev_close(s_spk);
        s_open = false;
    }
    if (esp_codec_dev_open(s_spk, &fs) != ESP_OK) return false;
    s_open = true;
    // simple settle and unmute
    vTaskDelay(pdMS_TO_TICKS(10));
    (void)esp_codec_dev_set_out_mute(s_spk, false);

    const size_t chunk_samp = 256 * channels; // interleaved frames
    size_t written = 0;
    while (written < samples) {
        size_t n = samples - written;
        if (n > chunk_samp) n = chunk_samp;
        if (esp_codec_dev_write(s_spk, (void*)(pcm + written), n * sizeof(int16_t)) != ESP_OK) break;
        written += n;
    }
    (void)esp_codec_dev_set_out_mute(s_spk, true);
    return true;
}

static bool play_wav_from_spiffs(const char *path)
{
    // Minimal WAV parser for PCM 16-bit LE mono/stereo
    struct stat st;
    if (stat(path, &st) != 0 || st.st_size <= 44) return false;
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    unsigned char hdr[44];
    size_t n = fread(hdr, 1, sizeof(hdr), f);
    if (n != sizeof(hdr)) { fclose(f); return false; }
    if (memcmp(hdr, "RIFF", 4) != 0 || memcmp(hdr + 8, "WAVE", 4) != 0) { fclose(f); return false; }
    // parse fmt chunk
    // Assuming standard layout: fmt chunk at 12, data chunk at or after 36
    uint16_t audio_format = *(uint16_t*)(hdr + 20);
    uint16_t num_channels = *(uint16_t*)(hdr + 22);
    uint32_t sample_rate  = *(uint32_t*)(hdr + 24);
    uint16_t bits_per_spl = *(uint16_t*)(hdr + 34);
    if (audio_format != 1 || (num_channels != 1 && num_channels != 2) || bits_per_spl != 16) {
        fclose(f);
        return false;
    }
    // Find data chunk (handle potential extra fmt bytes)
    uint32_t data_offset = 12;
    uint32_t data_size = 0;
    for (;;) {
        if (fseek(f, data_offset, SEEK_SET) != 0) { fclose(f); return false; }
        unsigned char chunk[8];
        if (fread(chunk, 1, 8, f) != 8) { fclose(f); return false; }
        uint32_t sz = *(uint32_t*)(chunk + 4);
        if (memcmp(chunk, "data", 4) == 0) { data_size = sz; data_offset += 8; break; }
        data_offset += 8 + sz;
        if (data_offset + 8 > (uint32_t)st.st_size) { fclose(f); return false; }
    }
    if (fseek(f, data_offset, SEEK_SET) != 0) { fclose(f); return false; }

    // Open codec with WAV format
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = (int)sample_rate,
        .channel = (int)num_channels,
        .bits_per_sample = 16,
    };
    int vol = (int)settings_get_notify_volume();
    if (vol < 0) vol = 0;
    if (vol > 100) vol = 100;
    esp_codec_dev_set_out_vol(s_spk, vol);
    if (s_open) {
        (void)esp_codec_dev_close(s_spk);
        s_open = false;
    }
    if (esp_codec_dev_open(s_spk, &fs) != ESP_OK) { fclose(f); return false; }
    s_open = true;
    vTaskDelay(pdMS_TO_TICKS(10));
    (void)esp_codec_dev_set_out_mute(s_spk, false);

    enum { BUF_SAMP = 1024*2 };
    int16_t *buf = (int16_t*)malloc(BUF_SAMP * sizeof(int16_t));
    if (!buf) { fclose(f); return false; }
    size_t remaining = data_size;
    while (remaining > 0) {
        size_t to_read_bytes = BUF_SAMP * sizeof(int16_t);
        if (to_read_bytes > remaining) to_read_bytes = remaining;
        size_t rn = fread(buf, 1, to_read_bytes, f);
        if (rn == 0) break;
        remaining -= rn;
        (void)esp_codec_dev_write(s_spk, buf, rn);
    }
    free(buf);
    fclose(f);
    (void)esp_codec_dev_set_out_mute(s_spk, true);
    return true;
}

void audio_alert_notify(void)
{
    if (!settings_get_sound()) return;
    // 1) Try to play preloaded file from SPIFFS (prefer notify.wav for now)
    if (play_wav_from_spiffs("/spiffs/notification.wav")) {
        return;
    }
    ESP_LOGI(TAG, "notification.wav not found, using synthesized tone");
    // Synthesize a bell-like "Dong": low base + partials, short pitch glide, multi-stage decay
    enum { SR = 22050 };
    const float base_f = 440.0f;     // base pitch, slightly lower for deeper "Dong"
    const float dur_s  = 0.32f;      // overall duration
    size_t N = (size_t)(SR * dur_s);
    static int16_t buf[8192];
    size_t max_samples = sizeof(buf) / sizeof(buf[0]);
    if (N > max_samples) {
        N = max_samples;
    }

    // Short fade-in to avoid click and give a percussive strike
    const float attack_s = 0.004f; // ~4ms
    const size_t attack_n = (size_t)(attack_s * SR);

    // Partial amplitudes and decays (sum kept < ~2.0 peak with early envelope)
    const float a0 = 1.00f, d0 = 5.0f;   // base, slower decay
    const float a1 = 0.45f, d1 = 8.0f;   // 1.5x, medium decay
    const float a2 = 0.25f, d2 = 12.0f;  // 2.0x, faster decay
    const float a3 = 0.20f, d3 = 10.0f;  // 2.0x detuned for gentle beating

    for (size_t n = 0; n < N; ++n) {
        float t = (float)n / (float)SR;

        // Slight downward glide: start ~6% sharp, relax to base quickly
        float glide = 1.0f + 0.06f * expf(-40.0f * t);
        float f0 = base_f * glide;
        float f1 = (base_f * 1.50f) * glide;
        float f2 = (base_f * 2.00f) * glide;
        float f3 = (base_f * 1.96f) * glide; // slight detune vs 2.0x

        // Exponential decays per partial
        float e0 = expf(-d0 * t);
        float e1 = expf(-d1 * t);
        float e2 = expf(-d2 * t);
        float e3 = expf(-d3 * t);

        // Sum of partials (bell-ish spectrum, simple model)
        float s = 0.0f;
        s += a0 * e0 * sinf(2.0f * 3.14159265f * f0 * t);
        s += a1 * e1 * sinf(2.0f * 3.14159265f * f1 * t);
        s += a2 * e2 * sinf(2.0f * 3.14159265f * f2 * t);
        s += a3 * e3 * sinf(2.0f * 3.14159265f * f3 * t);

        // Attack ramp
        float fade_in = 1.0f;
        if (n < attack_n) {
            fade_in = (float)n / (float)(attack_n);
        }
        s *= fade_in;

        // Normalize to safe headroom
        int v = (int)(s * 20000.0f);
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
