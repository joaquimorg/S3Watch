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
#include "freertos/timers.h"
#include "nimble-nordic-uart.h"
#include "rtc_lib.h"
#include "esp-bsp.h"
#include "sensors.h"
#include "esp_event.h"
#include "bsp/esp32_s3_touch_amoled_2_06.h"
#include "notifications.h"
#include "display_manager.h"
#include "ui.h"
#include "audio_alert.h"

static const char *TAG = "BLE_SYNC";

// Define event base for BLE connection status
ESP_EVENT_DEFINE_BASE(BLE_SYNC_EVENT_BASE);

// Track BLE connection state to gate periodic status updates
static volatile bool s_ble_connected = false;
static TimerHandle_t s_status_timer = NULL;

static void status_timer_cb(TimerHandle_t xTimer)
{
    (void)xTimer;
    if (s_ble_connected) {
        ble_sync_send_status(bsp_power_get_battery_percent(), bsp_power_is_charging());
    }
}

static void handle_notification_fields(const char* timestamp,
                                       const char* app,
                                       const char* title,
                                       const char* message)
{
    ESP_LOGI(TAG, "Notification: app='%s' title='%s' message='%s' ts='%s'",
             app ? app : "", title ? title : "", message ? message : "", timestamp ? timestamp : "");

    // Wake display for visibility
    display_manager_turn_on();
    if (bsp_display_lock(10)) {
        ui_show_messages_tile();
        notifications_show(app, title, message, timestamp);        
        bsp_display_unlock();
    }
    // Play notification sound if enabled
    audio_alert_notify();
}

static void process_one_json_object(const char* json, size_t len)
{
    // cJSON requires a C-string; ensure local null-terminated copy for parsing
    char* tmp = (char*)malloc(len + 1);
    if (!tmp) return;
    memcpy(tmp, json, len);
    tmp[len] = '\0';

    cJSON* root = cJSON_Parse(tmp);
    if (!root) {
        free(tmp);
        return;
    }

    // Existing handlers (datetime, notification, status)
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
        cJSON *app = cJSON_GetObjectItem(root, "app");
        cJSON *title = cJSON_GetObjectItem(root, "title");
        cJSON *message = cJSON_GetObjectItem(root, "message");
        const char* app_s = cJSON_IsString(app) ? app->valuestring : "";
        const char* title_s = cJSON_IsString(title) ? title->valuestring : "";
        const char* msg_s = cJSON_IsString(message) ? message->valuestring : "";
        handle_notification_fields(notification->valuestring, app_s, title_s, msg_s);
    }

    cJSON *status = cJSON_GetObjectItem(root, "status");
    if (cJSON_IsString(status)) {
        ESP_LOGI(TAG, "Status");
        ble_sync_send_status(bsp_power_get_battery_percent(), bsp_power_is_charging());
    }

    cJSON_Delete(root);
    free(tmp);
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

        ESP_LOGI(TAG, "Received chunk: %u bytes", (unsigned)item_size);
        ESP_LOGI(TAG, "Received buffer: %s", mbuf);

        process_one_json_object(mbuf, item_size);

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
        s_ble_connected = true;
        (void)esp_event_post(BLE_SYNC_EVENT_BASE, BLE_SYNC_EVT_CONNECTED, NULL, 0, 0);
        // Optionally send immediate status upon connect
        ble_sync_send_status(bsp_power_get_battery_percent(), bsp_power_is_charging());

        break;
    case NORDIC_UART_DISCONNECTED:
        ESP_LOGI(TAG, "Nordic UART disconnected");
        s_ble_connected = false;
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

    xTaskCreate(uartTask, "uartTask", 4000, NULL, 3, NULL);

    // Periodic status every 5 minutes when connected
    if (!s_status_timer) {
        s_status_timer = xTimerCreate("ble_status_5m", pdMS_TO_TICKS(5 * 60 * 1000), pdTRUE, NULL, status_timer_cb);
        if (s_status_timer) {
            xTimerStart(s_status_timer, 0);
        }
    }

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
