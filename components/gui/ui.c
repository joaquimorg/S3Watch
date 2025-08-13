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

    lv_display_t * display = lv_display_get_default();

    //theme_original = lv_display_get_theme(display);
    //lv_theme_t * theme = lv_theme_default_init(display, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
    //                                           true, LV_FONT_DEFAULT);
    //lv_display_set_theme(display, theme);

}


void ui_init(void) {
    bsp_display_lock(0);

    init_theme();

    watchface_create();
    //notifications_create();
    //settings_screen_create();
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
