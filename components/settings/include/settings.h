#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define SETTINGS_DISPLAY_TIMEOUT_10S 10000
#define SETTINGS_DISPLAY_TIMEOUT_20S 20000
#define SETTINGS_DISPLAY_TIMEOUT_30S 30000
#define SETTINGS_DISPLAY_TIMEOUT_1MIN 60000

void settings_init(void);
void settings_set_brightness(uint8_t level);
uint8_t settings_get_brightness(void);
void settings_set_display_timeout(uint32_t timeout);
uint32_t settings_get_display_timeout(void);
void settings_set_sound(bool enabled);
bool settings_get_sound(void);
void settings_set_bluetooth_enabled(bool enabled);
bool settings_get_bluetooth_enabled(void);

// Notification volume (0-100)
void settings_set_notify_volume(uint8_t vol_percent);
uint8_t settings_get_notify_volume(void);

// Persist settings to SPIFFS JSON and load from it
bool settings_save(void);
bool settings_load(void);

// Step goal (daily steps target)
void settings_set_step_goal(uint32_t steps);
uint32_t settings_get_step_goal(void);

// Restore factory defaults and persist
bool settings_reset_defaults(void);

// Maintenance: format SPIFFS storage partition
bool settings_format_spiffs(void);

#ifdef __cplusplus
}
#endif
