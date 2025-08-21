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

static const char* TAG = "UI";



lv_obj_t* active_screen;
lv_obj_t* mainTileView;
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

void load_screen(lv_obj_t* current_screen, lv_screen_load_anim_t anim) {
    if (active_screen != current_screen) {
        lv_screen_load_anim(current_screen, anim, 400, 0, false);
        active_screen = current_screen;
    }
};

void create_main_screen(void) {

    mainTileView = lv_tileview_create(NULL);
    lv_obj_set_width(mainTileView, lv_pct(100));
    lv_obj_set_height(mainTileView, lv_pct(100));
    lv_obj_set_scrollbar_mode(mainTileView, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(mainTileView, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
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

    // Create Steps screen under the Steps panel
    steps_screen_create(ui_Steps_Panel);

    lv_obj_set_tile_id(mainTileView, 0, 1, LV_ANIM_OFF);

    lv_disp_load_scr(mainTileView);
    active_screen = mainTileView;
}

void ui_show_messages_tile(void)
{
    if (!mainTileView) return;
    // Messages tile is at (0,0)
    lv_obj_set_tile_id(mainTileView, 0, 0, LV_ANIM_ON);
}




void ui_init(void) {
    bsp_display_lock(0);

    init_theme();

    create_main_screen();
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

    // Start sensors (QMI8658) for steps/activity and raise-to-wake
    sensors_init();
    xTaskCreate(sensors_task, "sensors_task", 4096, NULL, 5, NULL);

    bsp_display_unlock();
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

    // Periodic fallback: refresh power state every 5s in case no events fire
    lv_timer_t* t = lv_timer_create(power_poll_cb, 5000, NULL);
    // Trigger once immediately to avoid initial 0%
    lv_timer_ready(t);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
