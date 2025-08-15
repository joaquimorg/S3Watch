#include "ble_sync.h"
#include <stdint.h>
#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"
#include "nimble-nordic-uart.h"

static const char *TAG = "BLE_SYNC";

void uartTask(void *parameter) {
  static char mbuf[CONFIG_NORDIC_UART_MAX_LINE_LENGTH + 1];

  for (;;) {
    size_t item_size;
    if (nordic_uart_rx_buf_handle) {
      const char *item = (char *)xRingbufferReceive(nordic_uart_rx_buf_handle, &item_size, portMAX_DELAY);

      if (item) {
        int i;
        for (i = 0; i < item_size; ++i) {
          if (item[i] >= 'a' && item[i] <= 'z')
            mbuf[i] = item[i] - 0x20;
          else
            mbuf[i] = item[i];
        }
        mbuf[item_size] = '\0';

        //nordic_uart_sendln(mbuf);
        //puts(mbuf);
        ESP_LOGI(TAG, "%s", mbuf);
        vRingbufferReturnItem(nordic_uart_rx_buf_handle, (void *)item);
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
    nordic_uart_start("Espruino S3 Watch", nordic_uart_callback);
    
    xTaskCreate(uartTask, "uartTask", 4000, NULL, 5, NULL);

    return ESP_OK;
}