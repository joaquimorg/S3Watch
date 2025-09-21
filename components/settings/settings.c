#include "settings.h"
#include "esp_log.h"
#include "esp_err.h"
#include "bsp/display.h"
#include "bsp/esp32_s3_touch_amoled_2_06.h"
#include "bsp_board_extra.h"
#include "rtc_lib.h"
#include <time.h>
#include <stdio.h>
#include <sys/stat.h>
#include "esp_spiffs.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "cJSON.h"

static const char *TAG = "SETTINGS";
static uint8_t brightness = 30;
static uint32_t display_timeout_ms = 30000;
static bool sound_enabled = true;
static bool bluetooth_enabled = true;
static uint8_t notify_volume = 100; // percent 0..100 (louder default)
static uint32_t step_goal = 8000;
static bool spiffs_ready = false;

// Debounced save timer (to limit flash writes when sliders change)
static TimerHandle_t s_save_timer = NULL;
// Forward declarations for internal JSON IO
static bool settings_write_json(void);
static bool settings_read_json(void);
static void save_timer_cb(TimerHandle_t xTimer)
{
    (void)xTimer;
    (void)settings_write_json();
}
static void schedule_save(void)
{
    const TickType_t delay_ticks = pdMS_TO_TICKS(10000); // 10 seconds
    if (!s_save_timer) {
        s_save_timer = xTimerCreate("settings_save", delay_ticks, pdFALSE, NULL, save_timer_cb);
    }
    if (!s_save_timer) return;
    // Restart (or start) timer with 10s
    (void)xTimerStop(s_save_timer, 0);
    (void)xTimerChangePeriod(s_save_timer, delay_ticks, 0);
    (void)xTimerStart(s_save_timer, 0);
}

#define SETTINGS_PARTITION "storage"
#define SETTINGS_FILE      "/spiffs/settings.json"

static bool settings_mount_spiffs(void)
{
    if (spiffs_ready) return true;
    // First try to mount without formatting
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = SETTINGS_PARTITION,
        .max_files = 4,
        .format_if_mount_failed = false,
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret == ESP_OK) {
        size_t total = 0, used = 0;
        if (esp_spiffs_info(SETTINGS_PARTITION, &total, &used) == ESP_OK) {
            ESP_LOGI(TAG, "SPIFFS mounted: %u/%u bytes used", (unsigned)used, (unsigned)total);
        }
        spiffs_ready = true;
        return true;
    }
    // If mount failed, inform user and retry with format
    ESP_LOGW(TAG, "SPIFFS mount failed (%s). Formatting...", esp_err_to_name(ret));
    // Show a simple full-screen message while formatting, if LVGL is ready
    lv_obj_t* overlay = NULL;
    if (lv_disp_get_default() != NULL && bsp_display_lock(100)) {
        overlay = lv_obj_create(lv_layer_top());
        lv_obj_remove_style_all(overlay);
        lv_obj_set_size(overlay, lv_pct(100), lv_pct(100));
        lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, 0);
        lv_obj_t* label = lv_label_create(overlay);
        lv_label_set_text(label, "Initializing smartwatch...\nFormatting storage...");
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(label);
        lv_refr_now(NULL);
        bsp_display_unlock();
    }

    // Retry with format
    conf.format_if_mount_failed = true;
    ret = esp_vfs_spiffs_register(&conf);
    if (overlay && bsp_display_lock(100)) {
        lv_obj_del(overlay);
        overlay = NULL;
        lv_refr_now(NULL);
        bsp_display_unlock();
    }
    if (ret == ESP_OK) {
        size_t total = 0, used = 0;
        (void)esp_spiffs_info(SETTINGS_PARTITION, &total, &used);
        ESP_LOGI(TAG, "SPIFFS formatted and mounted: %u/%u bytes used", (unsigned)used, (unsigned)total);
        spiffs_ready = true;
        return true;
    }
    ESP_LOGE(TAG, "Failed to format+mount SPIFFS (%s)", esp_err_to_name(ret));
    return false;
}

static bool settings_write_json(void)
{
    if (!settings_mount_spiffs()) return false;
    cJSON *root = cJSON_CreateObject();
    if (!root) return false;
    cJSON_AddNumberToObject(root, "brightness", brightness);
    cJSON_AddNumberToObject(root, "display_timeout_ms", (double)display_timeout_ms);
    cJSON_AddBoolToObject(root, "sound_enabled", sound_enabled);
    cJSON_AddBoolToObject(root, "bluetooth_enabled", bluetooth_enabled);
    cJSON_AddNumberToObject(root, "notify_volume", (double)notify_volume);
    cJSON_AddNumberToObject(root, "step_goal", (double)step_goal);

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json_str) return false;

    FILE *f = fopen(SETTINGS_FILE, "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s for write", SETTINGS_FILE);
        cJSON_free(json_str);
        return false;
    }
    size_t n = fwrite(json_str, 1, strlen(json_str), f);
    fclose(f);
    cJSON_free(json_str);
    if (n == 0) {
        ESP_LOGE(TAG, "Failed to write settings to %s", SETTINGS_FILE);
        return false;
    }
    ESP_LOGI(TAG, "Settings saved to %s", SETTINGS_FILE);
    return true;
}

static bool settings_read_json(void)
{
    if (!settings_mount_spiffs()) return false;
    struct stat st;
    if (stat(SETTINGS_FILE, &st) != 0 || st.st_size <= 0) {
        ESP_LOGW(TAG, "Settings file not found; using defaults");
        return false;
    }
    FILE *f = fopen(SETTINGS_FILE, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s for read", SETTINGS_FILE);
        return false;
    }
    char *buf = (char*)malloc(st.st_size + 1);
    if (!buf) { fclose(f); return false; }
    size_t n = fread(buf, 1, st.st_size, f);
    fclose(f);
    buf[n] = '\0';
    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON settings");
        return false;
    }
    // Optional fields; keep defaults if missing
    cJSON *j;
    j = cJSON_GetObjectItem(root, "brightness");
    if (cJSON_IsNumber(j)) brightness = (uint8_t)j->valuedouble;
    j = cJSON_GetObjectItem(root, "display_timeout_ms");
    if (cJSON_IsNumber(j)) display_timeout_ms = (uint32_t)j->valuedouble;
    j = cJSON_GetObjectItem(root, "sound_enabled");
    if (cJSON_IsBool(j)) sound_enabled = cJSON_IsTrue(j);
    j = cJSON_GetObjectItem(root, "bluetooth_enabled");
    if (cJSON_IsBool(j)) bluetooth_enabled = cJSON_IsTrue(j);
    j = cJSON_GetObjectItem(root, "notify_volume");
    if (cJSON_IsNumber(j)) notify_volume = (uint8_t)j->valuedouble;
    j = cJSON_GetObjectItem(root, "step_goal");
    if (cJSON_IsNumber(j)) step_goal = (uint32_t)j->valuedouble;
    cJSON_Delete(root);
    // Apply to hardware where relevant
    bsp_display_brightness_set(brightness);
    ESP_LOGI(TAG, "Settings loaded: br=%u, to=%u, sound=%d", (unsigned)brightness, (unsigned)display_timeout_ms, (int)sound_enabled);
    return true;
}

void settings_init(void) {
    ESP_LOGI(TAG, "Settings init: mount + load JSON");
    (void)settings_load();
    // Ensure brightness is applied even if using defaults
    bsp_display_brightness_set(brightness);

    struct tm time;
    if (rtc_get_time(&time) == ESP_OK) {
        if (time.tm_year < 2025) { // struct tm year is years since 1900
            ESP_LOGI(TAG, "Time not set, setting to default");
            struct tm default_time = {
                .tm_year = 2025, // 2024
                .tm_mon = 1,    // January
                .tm_mday = 1,
                .tm_hour = 12,
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
    schedule_save();
}

uint8_t settings_get_brightness(void) {
    return brightness;
}

void settings_set_display_timeout(uint32_t timeout) {
    if (timeout == 10000 || timeout == 20000 || timeout == 30000 || timeout == 60000) {
        display_timeout_ms = timeout;
        schedule_save();
    }
}

uint32_t settings_get_display_timeout(void) {
    return display_timeout_ms;
}

void settings_set_sound(bool enabled) {
    sound_enabled = enabled;
    ESP_LOGI(TAG, "Sound %s", enabled ? "enabled" : "disabled");
    schedule_save();
}

bool settings_get_sound(void) {
    return sound_enabled;
}

void settings_set_bluetooth_enabled(bool enabled)
{
    if (bluetooth_enabled == enabled) {
        return;
    }
    bluetooth_enabled = enabled;
    ESP_LOGI(TAG, "Bluetooth %s", enabled ? "enabled" : "disabled");
    schedule_save();
}

bool settings_get_bluetooth_enabled(void)
{
    return bluetooth_enabled;
}

void settings_set_notify_volume(uint8_t vol_percent)
{
    if (vol_percent > 100) vol_percent = 100;
    notify_volume = vol_percent;
    schedule_save();
}

uint8_t settings_get_notify_volume(void)
{
    return notify_volume;
}

bool settings_save(void) {
    return settings_write_json();
}

bool settings_load(void) {
    return settings_read_json();
}

void settings_set_step_goal(uint32_t steps)
{
    if (steps < 1000) steps = 1000;
    if (steps > 100000) steps = 100000;
    step_goal = steps;
    schedule_save();
}

uint32_t settings_get_step_goal(void)
{
    return step_goal;
}

static void apply_defaults(void)
{
    brightness = 30;
    display_timeout_ms = 30000;
    sound_enabled = true;
    notify_volume = 100;
    step_goal = 8000;
    bluetooth_enabled = true;
}

bool settings_reset_defaults(void)
{
    apply_defaults();
    // Apply immediate effects
    bsp_display_brightness_set(brightness);
    return settings_write_json();
}

bool settings_format_spiffs(void)
{
    // Unregister if mounted
    if (spiffs_ready) {
        esp_vfs_spiffs_unregister(SETTINGS_PARTITION);
        spiffs_ready = false;
    }
    // Show formatting overlay to avoid white screen
    lv_obj_t* overlay = NULL;
    if (lv_disp_get_default() != NULL && bsp_display_lock(100)) {
        overlay = lv_obj_create(lv_layer_top());
        lv_obj_remove_style_all(overlay);
        lv_obj_set_size(overlay, lv_pct(100), lv_pct(100));
        lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, 0);
        lv_obj_t* label = lv_label_create(overlay);
        lv_label_set_text(label, "Formatting storage...");
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(label);
        lv_refr_now(NULL);
        bsp_display_unlock();
    }
    esp_err_t r = esp_spiffs_format(SETTINGS_PARTITION);
    if (r != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS format failed: %s", esp_err_to_name(r));
        if (overlay && bsp_display_lock(100)) { lv_obj_del(overlay); lv_refr_now(NULL); bsp_display_unlock(); }
        return false;
    }
    // Remount and write defaults
    bool ok = settings_mount_spiffs() && settings_write_json();
    if (overlay && bsp_display_lock(100)) { lv_obj_del(overlay); lv_refr_now(NULL); bsp_display_unlock(); }
    return ok;
}
