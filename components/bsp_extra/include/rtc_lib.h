#ifndef __RTC_H__
#define __RTC_H__

#include <time.h>
#include "esp_err.h"

esp_err_t rtc_start(void);
esp_err_t rtc_get_time(struct tm *time);
esp_err_t rtc_set_time(const struct tm *time);

int rtc_get_hour(void);
int rtc_get_minute(void);
int rtc_get_second(void);
int rtc_get_day(void);
int rtc_get_month(void);
int rtc_get_year(void);
const char *rtc_get_weekday_string(void);
const char *rtc_get_weekday_short_string(void);
const char *rtc_get_month_string(void);

#endif /* __RTC_H__ */
