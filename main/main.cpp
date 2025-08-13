#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_memory_utils.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "sdkconfig.h"


void sample_ui(void) {
    lv_obj_t * label1 = lv_label_create(lv_screen_active());
    lv_label_set_text(label1, "Welcome to LVGL !");
    lv_obj_align(label1, LV_ALIGN_CENTER, 0, 0);

    
    lv_obj_t * btn1 = lv_button_create(lv_screen_active());    
    lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -100);    

    lv_obj_t * label = lv_label_create(btn1);
    lv_label_set_text(label, "Hello LVGL!");
    lv_obj_center(label);
}


extern "C" void app_main(void) {

    bsp_display_start();
    bsp_display_brightness_set(80);

    bsp_display_lock(0);
    sample_ui();
    bsp_display_unlock();
}