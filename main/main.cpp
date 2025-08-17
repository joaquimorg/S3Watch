#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "bsp_board_extra.h"
#include "ble_sync.h"
#include "sensors.h"
#include "settings.h"
#include "ui.h"

static const char *TAG = "MAIN";

extern "C" void app_main(void) {
    
    bsp_display_start();

    bsp_extra_init();

    settings_init();

    sensors_init();

    xTaskCreate(ui_task, "ui", 4096, NULL, 5, NULL);
    xTaskCreate(sensors_task, "sensors", 4096, NULL, 5, NULL);
}
