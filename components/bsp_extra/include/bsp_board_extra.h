#pragma once

#include <sys/cdefs.h>
#include <stdbool.h>
#include "esp_codec_dev.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"


#ifdef __cplusplus
extern "C" {
#endif

esp_err_t bsp_extra_init(void);

#ifdef __cplusplus
}
#endif
