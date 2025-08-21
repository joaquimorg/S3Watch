#pragma once
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif    

    void lv_smartwatch_batt_create(lv_obj_t * screen);

    // Returns the battery screen object, creating it if necessary
    lv_obj_t* batt_screen_get(void);

#ifdef __cplusplus
}
#endif
