#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bsp/display.h"
#include "bsp/esp-bsp.h"
#include "bsp_board_extra.h"
#include "display_manager.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lvgl.h"
#include "sensors.h"
#include "settings.h"
#include "ui.h"
// Power management
#include "esp_wifi.h"
#include "esp_bt.h"
#include "esp_sleep.h"
#include "esp_pm.h"
// UI/BLE reagem a eventos de energia; remover ponte direta aqui
#include "audio_alert.h"
#include "ble_sync.h"

static const char *TAG = "MAIN";

/*
#define CONFIG_LV_USE_LOG 1
#define CONFIG_LV_LOG_LEVEL LV_LOG_LEVEL_INFO  // or DEBUG/TRACE

static void lvgl_log_cb(lv_log_level_t level, const char *buf)
{
    // buf usually already ends with '\n'; don't add another.
    switch (level) {
    case LV_LOG_LEVEL_ERROR: ESP_LOGE("LVGL", "%s", buf); break;
    case LV_LOG_LEVEL_WARN:  ESP_LOGW("LVGL", "%s", buf); break;
    case LV_LOG_LEVEL_USER:  // falls through to INFO
    case LV_LOG_LEVEL_INFO:  ESP_LOGI("LVGL", "%s", buf); break;
    //case LV_LOG_LEVEL_DEBUG: ESP_LOGD("LVGL", "%s", buf); break;
    case LV_LOG_LEVEL_TRACE: ESP_LOGV("LVGL", "%s", buf); break;
    default:                 ESP_LOGI("LVGL", "%s", buf); break;
    }
}*/

static void power_init(void) {
    esp_wifi_stop();
    esp_wifi_deinit();

    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT); // only BLE 
}

extern "C" void app_main(void) {

  // esp_log_level_set("lcd_panel.io.spi", ESP_LOG_DEBUG);

  //lv_log_register_print_cb(lvgl_log_cb);
  power_init();

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

  esp_err_t ble_cfg_err = ble_sync_set_enabled(settings_get_bluetooth_enabled());
  if (ble_cfg_err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to apply stored BLE state: %s", esp_err_to_name(ble_cfg_err));
  }

  // UI task chama display_manager_init() após criar o ecrã

  // UI e BLE subscrevem eventos diretamente; sem acoplamento no main

  //sensors_init();

  // Run the UI at a slightly higher priority so LVGL remains responsive
  xTaskCreate(ui_task, "ui", 8000, NULL, 4, NULL);
  
  // Sensor sampling can run at a lower priority without affecting UX
  //xTaskCreate(sensors_task, "sensors", 4096, NULL, 3, NULL);

  // Play a subtle startup tone once the system is up
  audio_alert_play_startup();

  // Now enable PM with light sleep allowed (still blocked while screen is ON)
  esp_pm_config_t pm_cfg = {
      .max_freq_mhz = 240,
      .min_freq_mhz = 80,
      .light_sleep_enable = true,
  };
  ESP_ERROR_CHECK(esp_pm_configure(&pm_cfg));
}
