#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

void settings_init(void);
void settings_set_brightness(uint8_t level);
uint8_t settings_get_brightness(void);
void settings_set_sound(bool enabled);
bool settings_get_sound(void);

#ifdef __cplusplus
}
#endif
