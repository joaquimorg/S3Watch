#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include <time.h>
#include <pcf85063a.h>

static const char *TAG = "PCF85063A";

extern int rtc_register_read(uint8_t regAddr, uint8_t *data, uint8_t len);
extern int rtc_register_write(uint8_t regAddr, uint8_t *data, uint8_t len);

static uint8_t dec_to_bcd(uint8_t val)
{
    return (val / 10 * 16) + (val % 10);
}

static uint8_t bcd_to_dec(uint8_t val)
{
    return (val / 16 * 10) + (val % 16);
}

/**
 * Calculate the day of the week for a given date using Zeller's congruence.
 *
 * @param day The day of the month (1 - 31).
 * @param month The month of the year (1 - 12).
 * @param year The year.
 * @return The day of the week, where 0 represents Sunday, 1 represents Monday, ..., 6 represents Saturday.
 *         Returns 0xFF if the input date is invalid.
 */
uint32_t getDayOfWeek(uint32_t day, uint32_t month, uint32_t year)
{
    if (day < 1 || day > 31 || month < 1 || month > 12) {
        return 0xFF;
    }
    if (month < 3) {
        month += 12;
        year--;
    }
    // Evaluate the parts of Zeller's congruence formula
    uint32_t part1 = day;
    uint32_t part2 = ((month + 1) * 26) / 10;
    uint32_t part3 = year;
    uint32_t part4 = year / 4;
    uint32_t part5 = 6 * (year / 100);
    uint32_t part6 = year / 400;

    uint32_t val = (part1 + part2 + part3 + part4 + part5 + part6) % 7;

    if (val == 0) {
        val = 7;
    }
    return val - 1;
}

esp_err_t pcf85063a_init(void) {
    uint8_t reg;
    esp_err_t ret = rtc_register_read(PCF85063A_CTRL1, &reg, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PCF85063A RTC READ FAILED!");
        return ESP_FAIL;
    }
    
    if ((reg & PCF85063A_CTRL1_STOP) != 0) {
        reg &= ~PCF85063A_CTRL1_STOP;
        ret = rtc_register_write(PCF85063A_CTRL1, &reg, 1);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start RTC");
            return ESP_FAIL;
        }
    }

    if ((reg & PCF85063A_CTRL1_12_24) != 0) {
        reg &= ~PCF85063A_CTRL1_12_24;
        ret = rtc_register_write(PCF85063A_CTRL1, &reg, 1);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set 24-hour mode");
            return ESP_FAIL;
        }
    }

    ESP_LOGI(TAG, "PCF85063A initialization complete");
    return ESP_OK;
}

esp_err_t pcf85063a_set_time(const struct tm *time)
{
    uint8_t time_buf[7];

    time_buf[0] = dec_to_bcd(time->tm_sec) & PCF85063A_SECONDS_MASK;
    time_buf[1] = dec_to_bcd(time->tm_min) & PCF85063A_MINUTES_MASK;
    time_buf[2] = dec_to_bcd(time->tm_hour) & PCF85063A_HOURS_MASK;
    time_buf[3] = dec_to_bcd(time->tm_mday) & PCF85063A_DAYS_MASK;
    time_buf[4] = getDayOfWeek(time->tm_mday, time->tm_mon, time->tm_year) & PCF85063A_WEEKDAYS_MASK;
    time_buf[5] = dec_to_bcd(time->tm_mon) & PCF85063A_MONTHS_MASK;
    time_buf[6] = dec_to_bcd(time->tm_year - 1900);

    return rtc_register_write(PCF85063A_SECONDS, time_buf, 7);
}

esp_err_t pcf85063a_get_time(struct tm *time)
{
    uint8_t time_buf[7];
    esp_err_t ret = rtc_register_read(PCF85063A_SECONDS, time_buf, 7);
    if (ret != ESP_OK) {
        return ret;
    }

    time->tm_sec = bcd_to_dec(time_buf[0] & PCF85063A_SECONDS_MASK);
    time->tm_min = bcd_to_dec(time_buf[1] & PCF85063A_MINUTES_MASK);
    time->tm_hour = bcd_to_dec(time_buf[2] & PCF85063A_HOURS_MASK);
    time->tm_mday = bcd_to_dec(time_buf[3] & PCF85063A_DAYS_MASK);
    time->tm_wday = bcd_to_dec(time_buf[4] & PCF85063A_WEEKDAYS_MASK);
    time->tm_mon = bcd_to_dec(time_buf[5] & PCF85063A_MONTHS_MASK) - 1;
    time->tm_year = bcd_to_dec(time_buf[6]) + 100;

    return ESP_OK;
}

esp_err_t pcf85063a_set_cap_sel(uint8_t cap_value)
{
    uint8_t reg;
    esp_err_t ret = rtc_register_read(PCF85063A_CTRL1, &reg, 1);
    if (ret != ESP_OK) {
        return ret;
    }

    reg &= ~PCF85063A_CTRL1_CAP_SEL;
    reg |= cap_value;

    return rtc_register_write(PCF85063A_CTRL1, &reg, 1);
}

esp_err_t pcf85063a_set_offset_mode(uint8_t offset_mode_value)
{
    uint8_t reg;
    esp_err_t ret = rtc_register_read(PCF85063A_OFFSET, &reg, 1);
    if (ret != ESP_OK) {
        return ret;
    }

    if (offset_mode_value) {
        reg |= PCF85063A_OFFSET_MODE;
    } else {
        reg &= ~PCF85063A_OFFSET_MODE;
    }

    return rtc_register_write(PCF85063A_OFFSET, &reg, 1);
}

esp_err_t pcf85063a_set_offset_value(uint8_t offset_value)
{
    uint8_t reg;
    esp_err_t ret = rtc_register_read(PCF85063A_OFFSET, &reg, 1);
    if (ret != ESP_OK) {
        return ret;
    }

    reg &= ~PCF85063A_OFFSET_VALUE_MASK;
    reg |= (offset_value & PCF85063A_OFFSET_VALUE_MASK);

    return rtc_register_write(PCF85063A_OFFSET, &reg, 1);
}