#include "display_manager.h"
#include "bsp/display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include "settings.h"
#include "esp_log.h"

#define DISPLAY_BUTTON GPIO_NUM_0

static const char *TAG = "DISPLAY_MGR";

static bool display_on = true;
static uint32_t timeout_ms;

static void display_turn_off_internal(void) {
    if (!display_on) {
        return;
    }
    ESP_LOGI(TAG, "Turning display off");
    bsp_display_brightness_set(0);
    display_on = false;
}

void display_manager_turn_off(void) {
    display_turn_off_internal();
}

void display_manager_turn_on(void) {
    if (!display_on) {
        ESP_LOGI(TAG, "Turning display on");
        bsp_display_brightness_set(settings_get_brightness());
        display_on = true;
    }
    display_manager_reset_timer();
}

bool display_manager_is_on(void) {
    return display_on;
}

void display_manager_reset_timer(void) {
    lv_disp_trig_activity(NULL);
}

static void touch_event_cb(lv_event_t *e) {
    display_manager_reset_timer();
}

static void display_manager_task(void *arg) {
    while (1) {
        if (display_on) {
            uint32_t inactive = lv_disp_get_inactive_time(NULL);
            if (inactive >= timeout_ms) {
                display_turn_off_internal();
            }
            if (gpio_get_level(DISPLAY_BUTTON) == 0) {
                display_manager_reset_timer();
                vTaskDelay(pdMS_TO_TICKS(500));
            }
        } else {
            if (gpio_get_level(DISPLAY_BUTTON) == 0) {
                display_manager_turn_on();
                vTaskDelay(pdMS_TO_TICKS(500));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void display_manager_init(void) {
    timeout_ms = settings_get_display_timeout();

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << DISPLAY_BUTTON,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    lv_obj_add_event_cb(lv_scr_act(), touch_event_cb, LV_EVENT_ALL, NULL);

    xTaskCreate(display_manager_task, "display_mgr", 2048, NULL, 5, NULL);
}

