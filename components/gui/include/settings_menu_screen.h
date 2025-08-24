#pragma once
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif

void settings_menu_screen_create(lv_obj_t* parent);
lv_obj_t* settings_menu_screen_get(void);

#ifdef __cplusplus
}
#endif
