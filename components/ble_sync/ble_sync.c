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
#include "sensors.h"

static const char *TAG = "BLE_SYNC";

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
          handle_notification(notification->valuestring);
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
        break;
    case NORDIC_UART_DISCONNECTED:
        ESP_LOGI(TAG, "Nordic UART disconnected");
        break;
    }
}

esp_err_t ble_sync_init(void)
{
    nordic_uart_start("ESP32 S3 Watch", nordic_uart_callback);

    xTaskCreate(uartTask, "uartTask", 4000, NULL, 5, NULL);

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