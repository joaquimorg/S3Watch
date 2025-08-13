#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"

#include "ble_service.h"
#include "sensors.h"
#include "settings.h"
#include "ui.h"

extern "C" void app_main(void) {
    bsp_display_start();
    settings_init();
    ble_init();
    sensors_init();

    xTaskCreate(ui_task, "ui", 4096, NULL, 5, NULL);
    xTaskCreate(ble_task, "ble", 4096, NULL, 5, NULL);
    xTaskCreate(sensors_task, "sensors", 4096, NULL, 5, NULL);
}
