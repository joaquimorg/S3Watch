#pragma once
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif

void storage_file_explorer_screen_create(lv_obj_t* parent);
lv_obj_t* storage_file_explorer_screen_get(void);

#ifdef __cplusplus
}
#endif
