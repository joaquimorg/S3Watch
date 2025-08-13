#include "ui.h"
#include "watchface.h"
#include "notifications.h"
#include "settings_screen.h"
#include "sensors.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "UI";

void ui_init(void) {
    watchface_create();
    notifications_create();
    settings_screen_create();
}

void ui_task(void *pvParameters) {
    ESP_LOGI(TAG, "UI task started");
    ui_init();
    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
