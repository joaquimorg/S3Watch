#include "unity.h"

#include "nimble-nordic-uart.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include <string.h>

TEST_CASE("buffer init / deinit", "[buffer]") {
  TEST_ESP_OK(_nordic_uart_buf_init());
  TEST_ESP_OK(_nordic_uart_buf_deinit());
  TEST_ESP_ERR(ESP_FAIL, _nordic_uart_buf_deinit());
}

TEST_CASE("buffer append", "[buffer]") {
  size_t item_size;
  char *str;

  TEST_ESP_OK(_nordic_uart_buf_init());
  TEST_ESP_OK(_nordic_uart_linebuf_append('a'));
  TEST_ESP_OK(_nordic_uart_linebuf_append('b'));
  TEST_ESP_OK(_nordic_uart_linebuf_append('c'));
  str = (char *)xRingbufferReceive(nordic_uart_rx_buf_handle, &item_size, 1);
  TEST_ASSERT(str == NULL);

  TEST_ESP_OK(_nordic_uart_linebuf_append('d'));
  TEST_ESP_OK(_nordic_uart_linebuf_append('\r'));
  TEST_ESP_OK(_nordic_uart_linebuf_append('\n'));
  str = (char *)xRingbufferReceive(nordic_uart_rx_buf_handle, &item_size, 1);
  TEST_ASSERT_EQUAL_STRING("abcd", str);

  vRingbufferReturnItem(nordic_uart_rx_buf_handle, str);

  TEST_ESP_OK(_nordic_uart_linebuf_append('\n'));
  str = (char *)xRingbufferReceive(nordic_uart_rx_buf_handle, &item_size, 1);
  TEST_ASSERT_EQUAL_STRING("", str);
  vRingbufferReturnItem(nordic_uart_rx_buf_handle, str);

  TEST_ESP_OK(_nordic_uart_buf_deinit());
}

TEST_CASE("line buffer overflow", "[buffer]") {
  size_t item_size;
  char *str;

  TEST_ESP_OK(_nordic_uart_buf_init());
  for (int i = 0; i < CONFIG_NORDIC_UART_MAX_LINE_LENGTH; ++i) {
    TEST_ESP_OK(_nordic_uart_linebuf_append('0' + i % 10));
  }
  TEST_ESP_ERR(ESP_FAIL, _nordic_uart_linebuf_append('d'));
  TEST_ESP_OK(_nordic_uart_linebuf_append('\r'));
  TEST_ESP_OK(_nordic_uart_linebuf_append('\n'));
  str = (char *)xRingbufferReceive(nordic_uart_rx_buf_handle, &item_size, 1);
  TEST_ASSERT_EQUAL_INT(CONFIG_NORDIC_UART_MAX_LINE_LENGTH, strlen(str));
  vRingbufferReturnItem(nordic_uart_rx_buf_handle, str);

  TEST_ESP_OK(_nordic_uart_linebuf_append('a'));
  TEST_ESP_OK(_nordic_uart_linebuf_append('\n'));
  str = (char *)xRingbufferReceive(nordic_uart_rx_buf_handle, &item_size, 1);
  TEST_ASSERT_EQUAL_STRING("a", str);
  vRingbufferReturnItem(nordic_uart_rx_buf_handle, str);

  TEST_ESP_OK(_nordic_uart_buf_deinit());
}

TEST_CASE("ring buffer overflow", "[buffer]") {
  size_t item_size;
  char *str;

  TEST_ESP_OK(_nordic_uart_buf_init());

  for (int j = 0; j < CONFIG_NORDIC_UART_RX_BUFFER_SIZE / (CONFIG_NORDIC_UART_MAX_LINE_LENGTH + 1); ++j) {
    for (int i = 0; i < CONFIG_NORDIC_UART_MAX_LINE_LENGTH; ++i) {
      TEST_ESP_OK(_nordic_uart_linebuf_append('0' + i % 10));
    }
    TEST_ESP_OK(_nordic_uart_linebuf_append('\n'));
  }

  // ring buffer overflow
  for (int i = 0; i < CONFIG_NORDIC_UART_MAX_LINE_LENGTH; ++i) {
    TEST_ESP_OK(_nordic_uart_linebuf_append('0' + i % 10));
  }
  TEST_ESP_ERR(ESP_FAIL, _nordic_uart_linebuf_append('\n'));

  for (int j = 0; j < CONFIG_NORDIC_UART_RX_BUFFER_SIZE / (CONFIG_NORDIC_UART_MAX_LINE_LENGTH + 1); ++j) {
    str = xRingbufferReceive(nordic_uart_rx_buf_handle, &item_size, 1);
    TEST_ASSERT_NOT_NULL(str);
    vRingbufferReturnItem(nordic_uart_rx_buf_handle, str);
  }
  TEST_ASSERT_NULL(xRingbufferReceive(nordic_uart_rx_buf_handle, &item_size, 1));

  TEST_ESP_OK(_nordic_uart_buf_deinit());
}
