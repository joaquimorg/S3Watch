#pragma once
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif

void notifications_screen_create(lv_obj_t* parent);
lv_obj_t* notifications_screen_get(void);

// Update the notifications UI with new data
// Any of the parameters may be NULL; they will be treated as empty strings
void notifications_show(const char* app,
                        const char* title,
                        const char* message,
                        const char* timestamp_iso8601);

#ifdef __cplusplus
}
#endif
