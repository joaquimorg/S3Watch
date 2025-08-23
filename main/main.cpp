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
// Power management
#include "esp_pm.h"
// UI/BLE reagem a eventos de energia; remover ponte direta aqui

static const char *TAG = "MAIN";

extern "C" void app_main(void) {
    
    // Create default event loop for component event handlers
    esp_event_loop_create_default();

    // Block light-sleep during boot and UI/display bring-up
    display_manager_pm_early_init();

    // Enable Dynamic Frequency Scaling + automatic light sleep so CPU idles low
    // BLE remains active; display_manager controls light-sleep via a PM lock
    // Defer PM config until after BSP and BLE init

    bsp_display_start();

    bsp_extra_init();

    settings_init();

    // UI task chama display_manager_init() após criar o ecrã

    // UI e BLE subscrevem eventos diretamente; sem acoplamento no main

    sensors_init();

    xTaskCreate(ui_task, "ui", 8000, NULL, 5, NULL);
    xTaskCreate(sensors_task, "sensors", 4096, NULL, 5, NULL);

    // Now enable PM with light sleep allowed (still blocked while screen is ON)
    esp_pm_config_t pm_cfg = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 80,
        .light_sleep_enable = true,
    };
    (void)esp_pm_configure(&pm_cfg);
}
