#include "ui.h"
#include "watchface.h"
#include "notifications.h"
#include "settings_screen.h"
#include "sensors.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h"

static const char *TAG = "UI";


static lv_theme_t * theme_original;


void init_theme(void) {

    //lv_display_t * display = lv_display_get_default();

    //theme_original = lv_display_get_theme(display);
    //lv_theme_t * theme = lv_theme_default_init(display, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
    //                                           true, LV_FONT_DEFAULT);
    //lv_display_set_theme(display, theme);

    lv_obj_remove_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);


}

lv_obj_t *label;  

void debug_screen(void) {
    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Initializing...");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}


void ui_init(void) {
    bsp_display_lock(0);

    init_theme();

    watchface_create();
    //notifications_create();
    //settings_screen_create();

    //debug_screen();

    bsp_display_unlock();
}

void ui_task(void *pvParameters) {
    ESP_LOGI(TAG, "UI task started");
    ui_init();
    while (1) {
        //lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
