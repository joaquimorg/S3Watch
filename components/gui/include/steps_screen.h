#pragma once
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif

void steps_screen_create(lv_obj_t* parent);
void steps_screen_set_goal(uint32_t goal_steps);

#ifdef __cplusplus
}
#endif
