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
static lv_obj_t* mainTileView;

lv_obj_t* tileClock;
lv_obj_t* tileConfig;
lv_obj_t* tileSteps;
lv_obj_t* tileMotfication;

lv_obj_t* ui_Clock01_Panel;
lv_obj_t* ui_Config_Panel;
lv_obj_t* ui_Steps_Panel;
lv_obj_t* ui_Messages_Panel;

lv_style_t main_style;

void init_theme(void) {

    lv_style_init(&main_style);
    lv_style_set_text_color(&main_style, lv_color_white());
    lv_style_set_bg_color(&main_style, lv_color_black());
    lv_style_set_border_color(&main_style, lv_color_black());
    //lv_style_set_bg_opa(&main_style, LV_OPA_100);

}

void load_screen(lv_obj_t* current_screen, lv_obj_t* next_screen, lv_screen_load_anim_t anim) {

    if (active_screen != next_screen) {
        lv_screen_load_anim(next_screen, anim, 200, 0, false);
        active_screen = next_screen;
        /*if (current_screen) {
            // Delete asynchronously to avoid heavy work in refresh/event context
            lv_obj_del_async(current_screen);
        }*/
    }
};

static void tile_change_event_cb(lv_event_t * e)
{
    lv_obj_t * tileview = lv_event_get_target(e);
    lv_obj_t * tile_act = lv_tileview_get_tile_act(tileview);

    if (tile_act == tileClock) {
        watchface_resume();
    } else {
        watchface_pause();
    }

    if (tile_act == tileSteps) {
        steps_screen_resume();
    } else {
        steps_screen_pause();
    }
}

void create_main_screen(void) {

    mainTileView = lv_tileview_create(NULL);
    lv_obj_set_width(mainTileView, lv_pct(100));
    lv_obj_set_height(mainTileView, lv_pct(100));
    lv_obj_set_scrollbar_mode(mainTileView, LV_SCROLLBAR_MODE_OFF);
    //lv_obj_add_flag(mainTileView, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_add_style(mainTileView, &main_style, 0);

    tileMotfication = lv_tileview_add_tile(mainTileView, 0, 0, LV_DIR_BOTTOM);
    ui_Messages_Panel = lv_obj_create(tileMotfication);
    lv_obj_add_style(ui_Messages_Panel, &main_style, 0);
    lv_obj_set_width(ui_Messages_Panel, lv_pct(100));
    lv_obj_set_height(ui_Messages_Panel, lv_pct(100));
    lv_obj_set_align(ui_Messages_Panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Messages_Panel, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
    lv_obj_set_scrollbar_mode(ui_Messages_Panel, LV_SCROLLBAR_MODE_OFF);

    tileClock = lv_tileview_add_tile(mainTileView, 0, 1, LV_DIR_BOTTOM | LV_DIR_TOP | LV_DIR_RIGHT);
    ui_Clock01_Panel = lv_obj_create(tileClock);
    lv_obj_add_style(ui_Clock01_Panel, &main_style, 0);
    lv_obj_set_width(ui_Clock01_Panel, lv_pct(100));
    lv_obj_set_height(ui_Clock01_Panel, lv_pct(100));
    lv_obj_set_align(ui_Clock01_Panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Clock01_Panel, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
    lv_obj_set_scrollbar_mode(ui_Clock01_Panel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(ui_Clock01_Panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);


    tileConfig = lv_tileview_add_tile(mainTileView, 0, 2, LV_DIR_TOP);
    ui_Config_Panel = lv_obj_create(tileConfig);
    lv_obj_add_style(ui_Config_Panel, &main_style, 0);
    lv_obj_set_width(ui_Config_Panel, lv_pct(100));
    lv_obj_set_height(ui_Config_Panel, lv_pct(100));
    lv_obj_set_align(ui_Config_Panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Config_Panel, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
    lv_obj_set_scrollbar_mode(ui_Config_Panel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(ui_Config_Panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);


    tileSteps = lv_tileview_add_tile(mainTileView, 1, 1, LV_DIR_LEFT);
    ui_Steps_Panel = lv_obj_create(tileSteps);
    lv_obj_add_style(ui_Steps_Panel, &main_style, 0);
    lv_obj_set_width(ui_Steps_Panel, lv_pct(100));
    lv_obj_set_height(ui_Steps_Panel, lv_pct(100));
    lv_obj_set_align(ui_Steps_Panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Steps_Panel, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
    lv_obj_set_scrollbar_mode(ui_Steps_Panel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(ui_Steps_Panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);


    watchface_create(ui_Clock01_Panel);

    lv_smartwatch_control_create(ui_Config_Panel);

    lv_smartwatch_notifications_create(ui_Messages_Panel);

    steps_screen_create(ui_Steps_Panel);


    lv_obj_set_tile_id(mainTileView, 0, 1, LV_ANIM_OFF);

    lv_obj_add_event_cb(mainTileView, tile_change_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_disp_load_scr(mainTileView);
    active_screen = mainTileView;
}

void ui_show_messages_tile(void)
{
    if (!mainTileView) return;
    // Messages tile is at (0,0)
    lv_obj_set_tile_id(mainTileView, 0, 0, LV_ANIM_ON);
}

lv_obj_t* ui_get_main_tileview(void)
{
    return mainTileView;
}




void ui_init(void) {
    bsp_display_lock(0);

    init_theme();

    // Register LVGL FS driver for SPIFFS before any file-based widgets
    lvgl_spiffs_fs_register();

    create_main_screen();
    watchface_resume();
    //watchface_create();
    //notifications_create();
    //settings_screen_create();

    //debug_screen();

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
    if (s_back_busy) return;
    s_back_busy = true;
    // Clear busy after short delay to avoid spamming back actions
    (void)lv_timer_create(clear_back_busy_cb, 250, NULL);
    lv_obj_t* scr = lv_scr_act();
    if (!scr) return;
    // If already at main tile, do nothing
    if (scr == mainTileView) return;
    // If flagged to go back to Storage tools (USER_3), handle first
    if (lv_obj_has_flag(scr, LV_OBJ_FLAG_USER_3)) {
        extern lv_obj_t* setting_storage_screen_get(void);
        load_screen(NULL, setting_storage_screen_get(), LV_SCR_LOAD_ANIM_MOVE_RIGHT);
        //lv_obj_del_async(scr);
        return;
    }
    // If flagged to go back to Settings menu (USER_1)
    if (lv_obj_has_flag(scr, LV_OBJ_FLAG_USER_1)) {
        extern void settings_menu_screen_create(lv_obj_t* parent);
        extern lv_obj_t* settings_menu_screen_get(void);
        // Ensure menu exists (persistent in Settings tile); fall back to getter
        load_screen(NULL, settings_menu_screen_get(), LV_SCR_LOAD_ANIM_MOVE_RIGHT);
        //lv_obj_del_async(scr);
        return;
    }
    // Default: go back to the main tileview
    load_screen(scr, mainTileView, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
    // Screens shown as full-screen should be lazily recreated next time
    //lv_obj_del_async(scr);
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
    xTaskCreate(ui_back_btn_task, "ui_back_btn", 2048, NULL, 5, NULL);

    // Periodic fallback: refresh power state every 5s in case no events fire
    lv_timer_t* t = lv_timer_create(power_poll_cb, 5000, NULL);
    // Trigger once immediately to avoid initial 0%
    lv_timer_ready(t);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
