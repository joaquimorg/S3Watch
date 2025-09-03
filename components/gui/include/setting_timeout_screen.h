#pragma once
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
void setting_timeout_screen_create(lv_obj_t* parent);
lv_obj_t* setting_timeout_screen_get(void);
#ifdef __cplusplus
}
#endif
