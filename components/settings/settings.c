#include "settings.h"
#include "esp_log.h"
#include "esp_err.h"
#include "bsp/display.h"

static const char *TAG = "SETTINGS";
static uint8_t brightness = 50;
static bool sound_enabled = true;

void settings_init(void) {
    ESP_LOGI(TAG, "Settings initialization placeholder");
    bsp_display_brightness_set(brightness);
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
