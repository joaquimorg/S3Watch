#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

void load_screen(lv_obj_t* current_screen, lv_obj_t* next_screen, lv_screen_load_anim_t anim);
lv_obj_t* active_screen_get(void);

void ui_init(void);
void ui_task(void *pvParameters);

// Switch to the Messages tile (notifications screen)
void ui_show_messages_tile(void);

// Accessor for the main TileView screen
//lv_obj_t* ui_get_main_tileview(void);

// Accessor for the main style
lv_style_t* ui_get_main_style(void);

#ifdef __cplusplus
}
#endif
