#pragma once
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif

esp_err_t audio_alert_init(void);
void audio_alert_notify(void);
// Schedule a one-shot startup tone after boot, with a short delay
// to allow the codec/PA to settle and avoid first-play clicks.
void audio_alert_play_startup(void);

#ifdef __cplusplus
}
#endif
