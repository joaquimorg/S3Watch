#pragma once
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif

// Create the brightness screen (safe to pass NULL for a full screen)
void lv_smartwatch_brightness_create(lv_obj_t* screen);

// Returns the brightness screen object, creating it if necessary
lv_obj_t* brightness_screen_get(void);

#ifdef __cplusplus
}
#endif

