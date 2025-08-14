#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "bsp/esp-bsp.h"
#include "bsp_board_extra.h"
#include "pcf85063a.h"
#include "ble_time_sync.h"

#define I2C_MASTER_TIMEOUT_MS 1000

static const char *TAG = "bsp_extra_board";

static i2c_master_bus_handle_t bus_handle;

static i2c_master_dev_handle_t rtc_dev_handle = NULL;

esp_err_t bsp_rtc_init(void)
{
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x51,
        .scl_speed_hz = CONFIG_I2C_MASTER_FREQUENCY,
        .scl_wait_us = 0,
        .flags = {
            .disable_ack_check = 0
        }
    };

    i2c_master_bus_add_device(bus_handle, &dev_config, &rtc_dev_handle);

    return ESP_OK;
}

// read function using new API
int rtc_register_read(uint8_t regAddr, uint8_t *data, uint8_t len) {
    esp_err_t ret = i2c_master_transmit_receive(rtc_dev_handle, &regAddr, 1, data, len, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "RTC READ FAILED!");
        return -1;
    }
    return 0;
}

// write function using new API
int rtc_register_write(uint8_t regAddr, uint8_t *data, uint8_t len) {
    uint8_t *buffer = (uint8_t *)malloc(len + 1);
    if (!buffer) return -1;
    buffer[0] = regAddr;
    memcpy(&buffer[1], data, len);

    esp_err_t ret = i2c_master_transmit(rtc_dev_handle, buffer, len + 1, I2C_MASTER_TIMEOUT_MS);
    free(buffer);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "RTC WRITE FAILED!");
        return -1;
    }
    return 0;
}

esp_err_t bsp_extra_init(void)
{
    esp_err_t ret;

    bus_handle = bsp_i2c_get_handle();

    ret = bsp_rtc_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "RTC init failed");
        return ret;
    }

    ret = pcf85063a_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PCF85063A init failed");
        return ret;
    }

    ret = ble_sync_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BLE sync init failed");
        return ret;
    }


    return ESP_OK;
}

