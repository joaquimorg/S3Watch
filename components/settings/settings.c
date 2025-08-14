#include "settings.h"
#include "esp_log.h"
#include "esp_err.h"
#include "bsp/display.h"
#include "bsp_board_extra.h"
#include "rtc_lib.h"
#include <time.h>

static const char *TAG = "SETTINGS";
static uint8_t brightness = 50;
static bool sound_enabled = true;

void settings_init(void) {
    ESP_LOGI(TAG, "Settings initialization placeholder");
    bsp_display_brightness_set(brightness);

    bsp_extra_init();

    struct tm time;
    if (rtc_get_time(&time) == ESP_OK) {
        if (time.tm_year < 123) { // struct tm year is years since 1900
            ESP_LOGI(TAG, "Time not set, setting to default");
            struct tm default_time = {
                .tm_year = 2025, // 2024
                .tm_mon = 8,    // January
                .tm_mday = 14,
                .tm_hour = 15,
                .tm_min = 0,
                .tm_sec = 0
            };
            rtc_set_time(&default_time);
        }
    }

    rtc_start();
}

void settings_set_brightness(uint8_t level) {
    brightness = level;
    bsp_display_brightness_set(brightness);
}

uint8_t settings_get_brightness(void) {
    return brightness;
}

void settings_set_sound(bool enabled) {
    sound_enabled = enabled;
    ESP_LOGI(TAG, "Sound %s", enabled ? "enabled" : "disabled");
}

bool settings_get_sound(void) {
    return sound_enabled;
}

