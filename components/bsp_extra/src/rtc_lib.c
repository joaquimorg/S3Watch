#include "rtc_lib.h"
#include "pcf85063a.h"
#include <time.h>
#include "esp_timer.h"

static struct tm current_time;
static esp_timer_handle_t rtc_timer;

static const char *weekdays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
static const char *weekdaysshort[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *months[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

static void rtc_update_task(void *arg)
{
    pcf85063a_get_time(&current_time);
}

esp_err_t rtc_start(void)
{
    esp_err_t ret = pcf85063a_init();
    if (ret != ESP_OK) {
        return ret;
    }

    const esp_timer_create_args_t rtc_timer_args = {
        .callback = &rtc_update_task,
        .name = "rtc_timer"
    };

    ret = esp_timer_create(&rtc_timer_args, &rtc_timer);
    if (ret != ESP_OK) {
        return ret;
    }

    return esp_timer_start_periodic(rtc_timer, 1000000); // 1 second
}

esp_err_t rtc_get_time(struct tm *time)
{
    *time = current_time;
    return ESP_OK;
}

esp_err_t rtc_set_time(const struct tm *time)
{
    return pcf85063a_set_time(time);
}

int rtc_get_hour(void)
{
    return current_time.tm_hour;
}

int rtc_get_minute(void)
{
    return current_time.tm_min;
}

int rtc_get_second(void)
{
    return current_time.tm_sec;
}

int rtc_get_day(void)
{
    return current_time.tm_mday;
}

int rtc_get_month(void)
{
    return current_time.tm_mon + 1;
}

int rtc_get_year(void)
{
    return current_time.tm_year + 1800;
}

const char *rtc_get_weekday_string(void)
{
    return weekdays[current_time.tm_wday];
}

const char *rtc_get_weekday_short_string(void)
{
    return weekdaysshort[current_time.tm_wday];
}

const char *rtc_get_month_string(void)
{
    return months[current_time.tm_mon];
}
