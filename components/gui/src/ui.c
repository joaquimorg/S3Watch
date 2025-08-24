#include "ui.h"
#include "watchface.h"
#include "steps_screen.h"
#include "notifications.h"
#include "settings_screen.h"
#include "sensors.h"
#include "display_manager.h"
#include "watchface.h"
#include "bsp/esp32_s3_touch_amoled_2_06.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "ble_sync.h"
#include "ui_fonts.h"

#include "batt_screen.h"
#include "lvgl_spiffs_fs.h"
#include "driver/gpio.h"
#include "bsp/esp32_s3_touch_amoled_2_06.h"

static const char* TAG = "UI";

static lv_obj_t* active_screen;
static volatile bool s_back_busy = false;
static void clear_back_busy_cb(lv_timer_t* t)
{
    (void)t;
    s_back_busy = false;
}
//static lv_obj_t* mainTileView;

//lv_obj_t* tileClock;
//lv_obj_t* tileConfig;
//lv_obj_t* tileSteps;
//lv_obj_t* tileMotfication;


//lv_obj_t* ui_Config_Panel;
//lv_obj_t* ui_Steps_Panel;
//lv_obj_t* ui_Messages_Panel;

static lv_style_t main_style;

void init_theme(void) {

    lv_style_init(&main_style);
    lv_style_set_text_color(&main_style, lv_color_white());
    lv_style_set_bg_color(&main_style, lv_color_black());
    lv_style_set_border_color(&main_style, lv_color_black());
    //lv_style_set_bg_opa(&main_style, LV_OPA_100);

}

lv_style_t* ui_get_main_style(void)
{
    return &main_style;
}

void load_screen(lv_obj_t* current_screen, lv_obj_t* next_screen, lv_screen_load_anim_t anim) {

    if (active_screen != next_screen) {
        //bsp_display_lock(0);
        lv_screen_load_anim(next_screen, anim, 200, 0, false);
        active_screen = next_screen;
        //bsp_display_unlock();
        /*if (current_screen) {
            // Delete asynchronously to avoid heavy work in refresh/event context
            lv_obj_del_async(current_screen);
        }*/
    }
};

lv_obj_t* active_screen_get(void)
{
    return active_screen;
}

void create_main_screen(void) {

    watchface_create();
    steps_screen_create();
    notifications_screen_create();
    control_screen_create();

    load_screen(NULL, watchface_screen_get(), LV_SCR_LOAD_ANIM_NONE);
}

void ui_show_messages_tile(void)
{
    load_screen(NULL, notifications_screen_get(), LV_SCR_LOAD_ANIM_OVER_TOP);
}


void ui_init(void) {
    bsp_display_lock(0);

    init_theme();

    // Register LVGL FS driver for SPIFFS before any file-based widgets
    lvgl_spiffs_fs_register();

    create_main_screen();

    {
        bool vbus = bsp_power_is_vbus_in();
        bool chg = bsp_power_is_charging();
        int  pct = bsp_power_get_battery_percent();
        watchface_set_power_state(vbus, chg, pct);
    }

    // Sensors are initialized and task started in main. Avoid duplicating here.

    bsp_display_unlock();
}

// Hardware back button (GPIO0) handler
#define UI_BACK_BTN GPIO_NUM_0

static void ui_handle_back_async(void* user)
{
    (void)user;

    lv_obj_t* scr = lv_scr_act();
    if (!scr) return;
    // If already at main tile, do nothing
    if (scr == watchface_screen_get()) return;

    load_screen(NULL, watchface_screen_get(), LV_SCR_LOAD_ANIM_OVER_TOP);

}

static void ui_back_btn_task(void* arg)
{
    (void)arg;
    // Configure GPIO0 as input with pull-up
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << UI_BACK_BTN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    (void)gpio_config(&io);
    int idle = gpio_get_level(UI_BACK_BTN); // consider this the idle (unpressed) level
    int prev = idle;
    TickType_t last_press = 0;
    const TickType_t debounce = pdMS_TO_TICKS(120);
    for (;;) {
        int lvl = gpio_get_level(UI_BACK_BTN);
        if (prev != lvl) {
            prev = lvl;
            // Treat a press as a transition away from idle level (works for active-low or active-high)
            if (lvl != idle) {
                TickType_t now = xTaskGetTickCount();
                if (now - last_press > debounce) {
                    last_press = now;
                    // Dispatch back action on LVGL thread
                    lv_async_call(ui_handle_back_async, NULL);
                }
            }
        }
        // When screen is ON, allow PMU power button short-press to act as Back.
        // Do NOT consume it when screen is OFF to preserve wake behavior.
        if (display_manager_is_on()) {
            if (bsp_power_poll_pwr_button_short()) {
                lv_async_call(ui_handle_back_async, NULL);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// Callback de eventos de energia (file-scope, não aninhada)
static void power_ui_evt(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
{
    (void)handler_arg;
    (void)base;
    (void)id;
    bsp_power_event_payload_t* pl = (bsp_power_event_payload_t*)event_data;
    if (pl && bsp_display_lock(10)) {
        int pct = bsp_power_get_battery_percent();
        watchface_set_power_state(pl->vbus_in, pl->charging, pct);
        bsp_display_unlock();
    }
}

// Callback BLE: atualiza ícone de ligação
static void ble_ui_evt(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
{
    (void)handler_arg; (void)base; (void)event_data;
    bool connected = (id == BLE_SYNC_EVT_CONNECTED);
    if (bsp_display_lock(10)) {
        watchface_set_ble_connected(connected);
        bsp_display_unlock();
    }
}

// Timer callback: periodic power refresh
static void power_poll_cb(lv_timer_t* t)
{
    (void)t;
    bool vbus = bsp_power_is_vbus_in();
    bool chg = bsp_power_is_charging();
    int  pct = bsp_power_get_battery_percent();
    if (bsp_display_lock(10)) {
        watchface_set_power_state(vbus, chg, pct);
        bsp_display_unlock();
    }
}

void ui_task(void* pvParameters) {
    ESP_LOGI(TAG, "UI task started");
    ui_init();
    display_manager_init();
    // Subscrever eventos de energia e atualizar UI
    esp_event_handler_register(BSP_POWER_EVENT_BASE, ESP_EVENT_ANY_ID, power_ui_evt, NULL);
    esp_event_handler_register(BLE_SYNC_EVENT_BASE, ESP_EVENT_ANY_ID, ble_ui_evt, NULL);

    // Start back button poller
    xTaskCreate(ui_back_btn_task, "ui_back_btn", 2048, NULL, 4, NULL);

    // Periodic fallback: refresh power state every 5s in case no events fire
    lv_timer_t* t = lv_timer_create(power_poll_cb, 5000, NULL);
    // Trigger once immediately to avoid initial 0%
    lv_timer_ready(t);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
