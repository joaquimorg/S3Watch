#pragma once
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif

void lv_smartwatch_notifications_create(lv_obj_t * screen);

// Update the notifications UI with new data
// Any of the parameters may be NULL; they will be treated as empty strings
void notifications_show(const char* app,
                        const char* title,
                        const char* message,
                        const char* timestamp_iso8601);

#ifdef __cplusplus
}
#endif
