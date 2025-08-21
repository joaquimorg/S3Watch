#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "display_manager.h"
#include "bsp/display.h"
#include "bsp/esp32_s3_touch_amoled_2_06.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "settings.h"
#include "nimble-nordic-uart.h"


// If the board provides simple GPIO buttons, use one as wake key.
// On this hardware BSP_CAPS_BUTTONS is 0, so we will use the PMU PWR key instead.
#define DISPLAY_BUTTON GPIO_NUM_0

static const char *TAG = "DISPLAY_MGR";

static bool display_on = true;
static uint32_t timeout_ms;

static void display_turn_off_internal(void) {
    if (!display_on) {
        return;
    }
    ESP_LOGI(TAG, "Turning display off");
    // Stop LVGL timers to pause flushing while panel sleeps
    lvgl_port_stop();
    // Disable touch input polling and optionally hold touch in reset
    lv_indev_t* indev = bsp_display_get_input_dev();
    if (indev) {
        lv_indev_enable(indev, false);
    }
#if defined(BSP_LCD_TOUCH_RST)
    gpio_set_direction(BSP_LCD_TOUCH_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(BSP_LCD_TOUCH_RST, 0);
#endif
    // Put panel into low-power sleep and ensure backlight is off
    bsp_display_sleep();
    bsp_display_brightness_set(0);
    // Hint BLE to prefer low-power connection parameters while screen is off
    nordic_uart_set_low_power_mode(true);
    display_on = false;
}

void display_manager_turn_off(void) {
    display_turn_off_internal();
}

void display_manager_turn_on(void) {
    if (!display_on) {
        ESP_LOGI(TAG, "Turning display on");
        // Wake the panel first, then resume LVGL and restore brightness        
        bsp_display_wake();
        lvgl_port_resume();
        bsp_display_brightness_set(settings_get_brightness());
        // Re-enable touch input and release touch reset
        lv_indev_t* indev = bsp_display_get_input_dev();
        if (indev) {
            lv_indev_enable(indev, true);
        }
#if defined(BSP_LCD_TOUCH_RST)
        gpio_set_direction(BSP_LCD_TOUCH_RST, GPIO_MODE_OUTPUT);
        gpio_set_level(BSP_LCD_TOUCH_RST, 1);
        vTaskDelay(pdMS_TO_TICKS(5));
#endif
        display_on = true;
    }
    // Restore more responsive BLE params when screen is on
    nordic_uart_set_low_power_mode(false);
    display_manager_reset_timer();
}

bool display_manager_is_on(void) {
    return display_on;
}

void display_manager_reset_timer(void) {
    lv_disp_trig_activity(NULL);
}

static void touch_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
        case LV_EVENT_PRESSED:
        case LV_EVENT_PRESSING:
        case LV_EVENT_RELEASED:
        case LV_EVENT_CLICKED:
        case LV_EVENT_LONG_PRESSED:
        case LV_EVENT_LONG_PRESSED_REPEAT:
        case LV_EVENT_GESTURE:
            display_manager_reset_timer();
            break;
        default:
            break; // ignore non-input/render events
    }
}

static bool wake_button_pressed(void)
{
#if BSP_CAPS_BUTTONS
    return gpio_get_level(DISPLAY_BUTTON) == 0;
#else
    // Poll AXP2101 power key short-press event
    return bsp_power_poll_pwr_button_short();
#endif
}

static void display_manager_task(void *arg) {
    ESP_LOGI(TAG, "Display manager task started");
    while (1) {
        if (display_on) {
            uint32_t inactive = lv_disp_get_inactive_time(NULL);
            if (inactive >= timeout_ms) {
                display_turn_off_internal();
            }
            if (wake_button_pressed()) {
                display_manager_reset_timer();
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        } else {
            if (wake_button_pressed()) {
                display_manager_turn_on();
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void display_manager_init(void) {
    timeout_ms = settings_get_display_timeout();
#if BSP_CAPS_BUTTONS
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << DISPLAY_BUTTON,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
#else
    ESP_LOGI(TAG, "Using PMU PWR key to wake display");
#endif

    lv_obj_add_event_cb(lv_scr_act(), touch_event_cb, LV_EVENT_ALL, NULL);

    xTaskCreate(display_manager_task, "display_mgr", 4000, NULL, 5, NULL);
}

