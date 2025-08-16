#ifndef __BLE_TIME_SYNC_H__
#define __BLE_TIME_SYNC_H__

#include "esp_err.h"

esp_err_t ble_sync_init(void);
esp_err_t ble_sync_send_status(int battery_percent, bool charging);

#endif /* __BLE_TIME_SYNC_H__ */
