#include "ble_sync.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "cJSON.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"
#include "nimble-nordic-uart.h"
#include "rtc_lib.h"
#include "esp-bsp.h"
#include "sensors.h"
#include "esp_event.h"
#include "bsp/esp32_s3_touch_amoled_2_06.h"

static const char *TAG = "BLE_SYNC";

// Define event base for BLE connection status
ESP_EVENT_DEFINE_BASE(BLE_SYNC_EVENT_BASE);

static void handle_notification(const char *message) {
  ESP_LOGI(TAG, "Notification: %s", message);
  // TODO: display notification on the screen
}

void uartTask(void *parameter) {
  static char mbuf[CONFIG_NORDIC_UART_MAX_LINE_LENGTH + 1];

  for (;;) {
    size_t item_size;
    if (nordic_uart_rx_buf_handle) {
      const char *item = (char *)xRingbufferReceive(nordic_uart_rx_buf_handle, &item_size, portMAX_DELAY);

      if (item) {
        memcpy(mbuf, item, item_size);
        mbuf[item_size] = '\0';
        vRingbufferReturnItem(nordic_uart_rx_buf_handle, (void *)item);

        
        ESP_LOGI(TAG, "Received: %s", mbuf);

        // Ignore control messages (e.g., disconnect marker)
        if (mbuf[0] == '\003') {
          continue;
        }

        cJSON *root = cJSON_Parse(mbuf);
        if (!root) {
          ESP_LOGW(TAG, "Invalid JSON: %s", mbuf);
          continue;
        }

        cJSON *datetime = cJSON_GetObjectItem(root, "datetime");
        if (cJSON_IsString(datetime)) {
          int year, month, day, hour, minute, second;
          if (sscanf(datetime->valuestring, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6) {
            struct tm t = {
                .tm_year = year,
                .tm_mon = month,
                .tm_mday = day,
                .tm_hour = hour,
                .tm_min = minute,
                .tm_sec = second};
            rtc_set_time(&t);
            ESP_LOGI(TAG, "RTC updated");
          }
        }

        cJSON *notification = cJSON_GetObjectItem(root, "notification");
        if (cJSON_IsString(notification)) {
          ESP_LOGI(TAG, "Notification");
          handle_notification(notification->valuestring);
        }

        cJSON *status = cJSON_GetObjectItem(root, "status");
        if (cJSON_IsString(status)) {
          //handle_notification(status->valuestring);
          ESP_LOGI(TAG, "Status");
          ble_sync_send_status(bsp_power_get_battery_percent(), bsp_power_is_charging());

        }

        cJSON_Delete(root);
      }
    } else {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }

  vTaskDelete(NULL);
}

static void nordic_uart_callback(enum nordic_uart_callback_type callback_type) {
    switch (callback_type) {
    case NORDIC_UART_CONNECTED:
        ESP_LOGI(TAG, "Nordic UART connected");
        (void)esp_event_post(BLE_SYNC_EVENT_BASE, BLE_SYNC_EVT_CONNECTED, NULL, 0, 0);

        break;
    case NORDIC_UART_DISCONNECTED:
        ESP_LOGI(TAG, "Nordic UART disconnected");
        (void)esp_event_post(BLE_SYNC_EVENT_BASE, BLE_SYNC_EVT_DISCONNECTED, NULL, 0, 0);
        break;
    }
}

// Enviar estado em cada evento de energia (file-scope C function, not nested)
static void power_ble_evt(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
{
    (void)handler_arg;
    (void)base;
    (void)id;
    bsp_power_event_payload_t* pl = (bsp_power_event_payload_t*)event_data;
    if (pl) {
        ble_sync_send_status(pl->battery_percent, pl->charging);
    }
}

esp_err_t ble_sync_init(void)
{
    esp_err_t err = nordic_uart_start("ESP32 S3 Watch", nordic_uart_callback);
    if (err != ESP_OK) {
        return err;
    }

    xTaskCreate(uartTask, "uartTask", 4000, NULL, 5, NULL);

    // Enviar estado em cada evento de energia
    esp_event_handler_register(BSP_POWER_EVENT_BASE, ESP_EVENT_ANY_ID, power_ble_evt, NULL);

    return ESP_OK;
}

esp_err_t ble_sync_send_status(int battery_percent, bool charging)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return ESP_FAIL;
    }

    cJSON_AddNumberToObject(root, "battery", battery_percent);
    cJSON_AddBoolToObject(root, "charging", charging);
    // Include VBUS presence for richer client status
    bool vbus = (bsp_power_get_vbus_voltage_mv() > 0);
    cJSON_AddBoolToObject(root, "vbus", vbus);
    cJSON_AddNumberToObject(root, "steps", sensors_get_step_count());

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json_str) {
        return ESP_FAIL;
    }

    esp_err_t err = nordic_uart_sendln(json_str);
    free(json_str);
    return err;
}
