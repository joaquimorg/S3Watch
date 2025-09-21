#ifndef __BLE_TIME_SYNC_H__
#define __BLE_TIME_SYNC_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ble_sync_init(void);
esp_err_t ble_sync_send_status(int battery_percent, bool charging);
esp_err_t ble_sync_set_enabled(bool enabled);
bool ble_sync_is_enabled(void);

// BLE connection status events for UI/other components
// Event base published by ble_sync component
ESP_EVENT_DECLARE_BASE(BLE_SYNC_EVENT_BASE);

typedef enum {
    BLE_SYNC_EVT_CONNECTED = 1,
    BLE_SYNC_EVT_DISCONNECTED = 2,
} ble_sync_event_id_t;

#ifdef __cplusplus
}
#endif

#endif /* __BLE_TIME_SYNC_H__ */
