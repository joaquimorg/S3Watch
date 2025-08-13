#include "sensors.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "SENSORS";
static uint32_t step_count = 0;

void sensors_init(void) {
    ESP_LOGI(TAG, "Sensors initialization placeholder");
}

uint32_t sensors_get_step_count(void) {
    return step_count;
}

void sensors_task(void *pvParameters) {
    ESP_LOGI(TAG, "Sensors task started");
    while (1) {
        step_count++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
