#include "nimble-nordic-uart.h"

#include <esp_log.h>
#include <esp_nimble_hci.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <nvs_flash.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>

static const char *_TAG = "NORDIC UART";

// #define CONFIG_NORDIC_UART_MAX_LINE_LENGTH 256
// #define CONFIG_NORDIC_UART_RX_BUFFER_SIZE 4096

// Split the message in BLE_SEND_MTU and send it.
esp_err_t nordic_uart_send(const char *message) { //
  return _nordic_uart_send(message);
}

esp_err_t nordic_uart_sendln(const char *message) {
  if (nordic_uart_send(message) != ESP_OK)
    return ESP_FAIL;
  if (nordic_uart_send("\r\n") != ESP_OK)
    return ESP_FAIL;
  return ESP_OK;
}

esp_err_t nordic_uart_start(const char *device_name, void (*callback)(enum nordic_uart_callback_type callback_type)) {
  return _nordic_uart_start(device_name, callback);
}

esp_err_t nordic_uart_stop(void) { //
  return _nordic_uart_stop();
}
