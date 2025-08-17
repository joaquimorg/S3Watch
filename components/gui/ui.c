#include "ui.h"
#include "watchface.h"
#include "notifications.h"
#include "settings_screen.h"
#include "sensors.h"
#include "display_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h"

static const char* TAG = "UI";


static lv_theme_t* theme_original;


void init_theme(void) {

    //lv_display_t * display = lv_display_get_default();

    //theme_original = lv_display_get_theme(display);
    //lv_theme_t * theme = lv_theme_default_init(display, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
    //                                           true, LV_FONT_DEFAULT);
    //lv_display_set_theme(display, theme);

    lv_obj_remove_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);


}



lv_obj_t* active_screen;
lv_obj_t* mainTileView;
lv_obj_t* tileClock;
lv_obj_t* tileConfig;
lv_obj_t* tileSteps;
lv_obj_t* tileMotfication;

lv_obj_t* ui_Clock01_Panel;
lv_obj_t* ui_Config_Panel;
lv_obj_t* ui_Steps_Panel;
lv_obj_t* ui_Messages_Panel;

lv_style_t main_style;

void load_screen(lv_obj_t* current_screen, lv_screen_load_anim_t anim) {
    if (active_screen != current_screen) {
        lv_screen_load_anim(current_screen, anim, 400, 0, false);
        active_screen = current_screen;
    }
};

void create_main_screen(void) {

    lv_style_init(&main_style);
    lv_style_set_text_color(&main_style, lv_color_white());
    lv_style_set_bg_color(&main_style, lv_color_black());
    lv_style_set_bg_opa(&main_style, LV_OPA_100);

    mainTileView = lv_tileview_create(NULL);
    lv_obj_set_width(mainTileView, lv_pct(100));
    lv_obj_set_height(mainTileView, lv_pct(100));
    lv_obj_set_scrollbar_mode(mainTileView, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(mainTileView, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
    //lv_obj_set_style_bg_color(mainTileView, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(mainTileView, &main_style, 0);

    tileMotfication = lv_tileview_add_tile(mainTileView, 0, 0, LV_DIR_BOTTOM);
    ui_Messages_Panel = lv_obj_create(tileMotfication);
    lv_obj_add_style(ui_Messages_Panel, &main_style, 0);
    lv_obj_set_width(ui_Messages_Panel, lv_pct(100));
    lv_obj_set_height(ui_Messages_Panel, lv_pct(100));
    lv_obj_set_align(ui_Messages_Panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Messages_Panel, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
    lv_obj_set_scrollbar_mode(ui_Messages_Panel, LV_SCROLLBAR_MODE_OFF);
    //lv_obj_set_style_radius(ui_Messages_Panel, 80, LV_PART_MAIN | LV_STATE_DEFAULT);
    //lv_obj_set_style_bg_color(ui_Messages_Panel, lv_color_hex(0x570057), LV_PART_MAIN | LV_STATE_DEFAULT);
    //lv_obj_set_style_bg_color(ui_Messages_Panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    tileClock = lv_tileview_add_tile(mainTileView, 0, 1, LV_DIR_BOTTOM | LV_DIR_TOP | LV_DIR_RIGHT);
    ui_Clock01_Panel = lv_obj_create(tileClock);
    lv_obj_set_width(ui_Clock01_Panel, lv_pct(100));
    lv_obj_set_height(ui_Clock01_Panel, lv_pct(100));
    lv_obj_set_align(ui_Clock01_Panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Clock01_Panel, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
    lv_obj_set_scrollbar_mode(ui_Clock01_Panel, LV_SCROLLBAR_MODE_OFF);
    //lv_obj_set_style_radius(ui_Clock01_Panel, 80, LV_PART_MAIN | LV_STATE_DEFAULT);
    //lv_obj_set_style_bg_color(ui_Clock01_Panel, lv_color_hex(0x570000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Clock01_Panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    tileConfig = lv_tileview_add_tile(mainTileView, 0, 2, LV_DIR_TOP);
    ui_Config_Panel = lv_obj_create(tileConfig);
    lv_obj_set_width(ui_Config_Panel, lv_pct(100));
    lv_obj_set_height(ui_Config_Panel, lv_pct(100));
    lv_obj_set_align(ui_Config_Panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Config_Panel, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
    lv_obj_set_scrollbar_mode(ui_Config_Panel, LV_SCROLLBAR_MODE_OFF);
    //lv_obj_set_style_radius(ui_Config_Panel, 80, LV_PART_MAIN | LV_STATE_DEFAULT);
    //lv_obj_set_style_bg_color(ui_Config_Panel, lv_color_hex(0x005700), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Config_Panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    tileSteps = lv_tileview_add_tile(mainTileView, 1, 1, LV_DIR_LEFT);
    ui_Steps_Panel = lv_obj_create(tileSteps);
    lv_obj_set_width(ui_Steps_Panel, lv_pct(100));
    lv_obj_set_height(ui_Steps_Panel, lv_pct(100));
    lv_obj_set_align(ui_Steps_Panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Steps_Panel, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
    lv_obj_set_scrollbar_mode(ui_Steps_Panel, LV_SCROLLBAR_MODE_OFF);
    //lv_obj_set_style_radius(ui_Steps_Panel, 80, LV_PART_MAIN | LV_STATE_DEFAULT);
    //lv_obj_set_style_bg_color(ui_Steps_Panel, lv_color_hex(0x000057), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Steps_Panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);




    /*lv_obj_t* label = lv_label_create(ui_Clock01_Panel);
    lv_label_set_text(label, "Clock");
    lv_obj_center(label);*/

    watchface_create(ui_Clock01_Panel);

    lv_obj_t* label2 = lv_label_create(ui_Config_Panel);
    lv_label_set_text(label2, "Config");
    lv_obj_center(label2);

    lv_obj_t* label3 = lv_label_create(ui_Steps_Panel);
    lv_label_set_text(label3, "Steps");
    lv_obj_center(label3);

    lv_obj_t* label4 = lv_label_create(ui_Messages_Panel);
    lv_label_set_text(label4, "Messages");
    lv_obj_center(label4);

    lv_obj_set_tile_id(mainTileView, 0, 1, LV_ANIM_OFF);

    lv_disp_load_scr(mainTileView);
    active_screen = mainTileView;
}




void ui_init(void) {
    bsp_display_lock(0);

    init_theme();

    create_main_screen();
    //watchface_create();
    //notifications_create();
    //settings_screen_create();

    //debug_screen();

    bsp_display_unlock();
}

void ui_task(void* pvParameters) {
    ESP_LOGI(TAG, "UI task started");
    ui_init();
    display_manager_init();
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
