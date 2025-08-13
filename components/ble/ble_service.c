#include "ble_service.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BLE";

void ble_init(void) {
    ESP_LOGI(TAG, "BLE initialization placeholder");
}

void ble_task(void *pvParameters) {
    ESP_LOGI(TAG, "BLE task started");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
