#include "nimble-nordic-uart.h"

#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>

static const char *_TAG = "NORDIC UART";

// the ringbuffer is an interface with external
RingbufHandle_t nordic_uart_rx_buf_handle;

static char *_nordic_uart_rx_line_buf = NULL;
static size_t _nordic_uart_rx_line_buf_pos = 0;

esp_err_t _nordic_uart_send_line_buf_to_ring_buf() {
  _nordic_uart_rx_line_buf[_nordic_uart_rx_line_buf_pos] = '\0';
  UBaseType_t res = xRingbufferSend(nordic_uart_rx_buf_handle, _nordic_uart_rx_line_buf,
                                    _nordic_uart_rx_line_buf_pos + 1, pdMS_TO_TICKS(100));
  _nordic_uart_rx_line_buf_pos = 0;

  return res == pdTRUE ? ESP_OK : ESP_FAIL;
}

esp_err_t _nordic_uart_linebuf_append(char c) {
  switch (c) {
  // break \003 == Ctrl-c
  case '\003':
    _nordic_uart_rx_line_buf[0] = '\003';
    _nordic_uart_rx_line_buf_pos = 1;
    if (_nordic_uart_send_line_buf_to_ring_buf() != ESP_OK) {
      ESP_LOGE(_TAG, "Failed to send item");
      return ESP_FAIL;
    }
    break;

  // skip \r
  case '\r':
    break;

  // send a line buffer to ring buffer
  case '\n':
  case '\0':
    if (_nordic_uart_send_line_buf_to_ring_buf() != ESP_OK) {
      ESP_LOGE(_TAG, "Failed to send item");
      return ESP_FAIL;
    }
    break;

  // push char to local line buffer
  default:
    if (_nordic_uart_rx_line_buf_pos < CONFIG_NORDIC_UART_MAX_LINE_LENGTH) {
      _nordic_uart_rx_line_buf[_nordic_uart_rx_line_buf_pos++] = c;
    } else {
      ESP_LOGE(_TAG, "line buffer overflow");
      return ESP_FAIL;
    }
    break;
  }
  return ESP_OK;
}

esp_err_t _nordic_uart_buf_deinit() {
  if (!_nordic_uart_linebuf_initialized())
    return ESP_FAIL;

  free(_nordic_uart_rx_line_buf);
  _nordic_uart_rx_line_buf = NULL;
  _nordic_uart_rx_line_buf_pos = 0;

  vRingbufferDelete(nordic_uart_rx_buf_handle);
  nordic_uart_rx_buf_handle = NULL;

  return ESP_OK;
}

esp_err_t _nordic_uart_buf_init() {
  _nordic_uart_buf_deinit();

  // Buffer for receive BLE and split it with /\r*\n/
  _nordic_uart_rx_line_buf = malloc(CONFIG_NORDIC_UART_MAX_LINE_LENGTH + 1);
  _nordic_uart_rx_line_buf_pos = 0;
  nordic_uart_rx_buf_handle = xRingbufferCreate(CONFIG_NORDIC_UART_RX_BUFFER_SIZE, RINGBUF_TYPE_NOSPLIT);
  if (nordic_uart_rx_buf_handle == NULL) {
    ESP_LOGE(_TAG, "Failed to create ring buffer");
    return ESP_FAIL;
  }
  return ESP_OK;
}

bool _nordic_uart_linebuf_initialized() { //
  return _nordic_uart_rx_line_buf != NULL;
}