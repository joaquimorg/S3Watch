#pragma once

#include <stdint.h>

#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>
#include <host/ble_hs.h>


#ifdef __cplusplus
extern "C" {
#endif

// Handle for the Nordic UART RX ring buffer
extern RingbufHandle_t nordic_uart_rx_buf_handle;

// Enum for Nordic UART callback types
enum nordic_uart_callback_type {
  NORDIC_UART_DISCONNECTED, // Callback type when disconnected
  NORDIC_UART_CONNECTED,    // Callback type when connected
};

// Type definition for UART receive callback function
typedef void (*uart_receive_callback_t)(struct ble_gatt_access_ctxt *ctxt);

// Function to start the Nordic UART service
// - device_name: Name of the BLE device
// - callback: Function pointer to the callback function
esp_err_t nordic_uart_start(const char *device_name, void (*callback)(enum nordic_uart_callback_type callback_type));

// Function to stop the Nordic UART service
esp_err_t nordic_uart_stop(void);
esp_err_t nordic_uart_disconnect(void);
esp_err_t nordic_uart_set_advertising_enabled(bool enable);

// Function to send a message over Nordic UART
// - message: String message to be sent
esp_err_t nordic_uart_send(const char *message);

// Function to send a message with a newline over Nordic UART
// - message: String message to be sent
esp_err_t nordic_uart_sendln(const char *message);

// Function to yield for UART receive callback
// - uart_receive_callback: Callback function for UART receive
esp_err_t nordic_uart_yield(uart_receive_callback_t uart_receive_callback);

// private funcs for testing.
esp_err_t _nordic_uart_buf_deinit();
esp_err_t _nordic_uart_buf_init();
esp_err_t _nordic_uart_send_line_buf_to_ring_buf();
esp_err_t _nordic_uart_linebuf_append(char c);
bool _nordic_uart_linebuf_initialized();
char* _nordic_uart_get_linebuf(void);

esp_err_t _nordic_uart_start(const char *device_name, void (*callback)(enum nordic_uart_callback_type callback_type));
esp_err_t _nordic_uart_stop(void);
esp_err_t _nordic_uart_send(const char *message);

// Hint to adjust connection parameters for power saving while keeping link alive.
// When enabled, prefers longer intervals and higher slave latency.
// Safe to call anytime; takes effect on the next connection or immediately if connected.
void nordic_uart_set_low_power_mode(bool enable);

#ifdef __cplusplus
}
#endif
