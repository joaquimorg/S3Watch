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

#ifdef __cplusplus
}
#endif
