#pragma once
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif

    void control_screen_create(lv_obj_t* parent);
    lv_obj_t* control_screen_get(void);

#ifdef __cplusplus
}
#endif
