#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "bsp_board_extra.h"
#include "sensors.h"
#include "settings.h"
#include "ui.h"
#include "display_manager.h"
#include "esp_event.h"
// UI/BLE reagem a eventos de energia; remover ponte direta aqui

static const char *TAG = "MAIN";

extern "C" void app_main(void) {
    
    // Create default event loop for component event handlers
    esp_event_loop_create_default();

    bsp_display_start();

    bsp_extra_init();

    settings_init();

    // UI task chama display_manager_init() após criar o ecrã

    // UI e BLE subscrevem eventos diretamente; sem acoplamento no main

    sensors_init();

    xTaskCreate(ui_task, "ui", 4096, NULL, 5, NULL);
    xTaskCreate(sensors_task, "sensors", 4096, NULL, 5, NULL);
}
