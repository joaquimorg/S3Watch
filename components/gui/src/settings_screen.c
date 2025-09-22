#include "settings_screen.h"
#include "settings.h"
#include "ui_fonts.h"
#include "batt_screen.h"
#include "brightness_screen.h"
#include "setting_flashlight_screen.h"
#include "esp_log.h"
#include "settings_menu_screen.h"
#include "rtc_lib.h"
#include "ble_sync.h"
#include "esp_err.h"

#include "ui.h"
#include "watchface.h"
#include "bsp/esp32_s3_touch_amoled_2_06.h"

static const char* TAG = "SETTINGS SCREEN";

// Use accessor from batt_screen instead of extern symbol

LV_IMAGE_DECLARE(image_mute_icon);
LV_IMAGE_DECLARE(image_flashlight_icon);
LV_IMAGE_DECLARE(image_brightness_icon);
LV_IMAGE_DECLARE(image_battery_icon);
LV_IMAGE_DECLARE(image_silence_icon);
LV_IMAGE_DECLARE(image_bluetooth_icon);
LV_IMAGE_DECLARE(image_settings_icon);

static void click_event_cb(lv_event_t* e);
static void toggle_event_cb(lv_event_t* e);
static void time_timer_cb(lv_timer_t* timer);
static void update_time_label(void);
static void control_screen_on_delete(lv_event_t* e);

static lv_obj_t* control_screen;
static lv_obj_t* time_label;
static lv_timer_t* time_timer;

static const lv_image_dsc_t* control_icons[] = {
    &image_brightness_icon,
    &image_silence_icon,
    &image_flashlight_icon,
    &image_battery_icon,
    &image_bluetooth_icon,
    &image_settings_icon
};

static const char* control_labels[] = {
    "Brightness",
    "Silence",
    "Flashlight",
    "Battery",
    "Bluetooth",
    "Settings"
};

enum control_id {
    CTRL_BRIGHTNESS = 0,
    CTRL_SILENCE,
    CTRL_FLASHLIGHT,
    CTRL_BATTERY,
    CTRL_BLUETOOTH,
    CTRL_SETTINGS,
};

static void update_time_label(void)
{
    if (!time_label) {
        return;
    }
    lv_label_set_text_fmt(time_label, "%02d:%02d", rtc_get_hour(), rtc_get_minute());
}

static void time_timer_cb(lv_timer_t* timer)
{
    (void)timer;
    bool locked = bsp_display_lock(0);
    update_time_label();
    if (locked) {
        bsp_display_unlock();
    }
}

static void control_screen_on_delete(lv_event_t* e)
{
    (void)e;
    if (time_timer) {
        lv_timer_del(time_timer);
        time_timer = NULL;
    }
    time_label = NULL;
    control_screen = NULL;
}

static void screen_events(lv_event_t* e);

static void screen_events(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
        if (dir == LV_DIR_BOTTOM) {
            lv_indev_wait_release(lv_indev_active());
            load_screen(control_screen, watchface_screen_get(), LV_SCR_LOAD_ANIM_MOVE_BOTTOM);
        }
    }
    else if (lv_event_get_code(e) == LV_EVENT_SCREEN_LOADED) {
        update_time_label();
        if (time_timer) {
            lv_timer_ready(time_timer);
        }
    }
}

void control_screen_create(lv_obj_t* parent)
{

    static lv_style_t cmain_style;


    lv_style_init(&cmain_style);
    lv_style_set_text_color(&cmain_style, lv_color_white());
    lv_style_set_bg_color(&cmain_style, lv_color_hex(0x000000));
    lv_style_set_bg_opa(&cmain_style, LV_OPA_100);

    control_screen = lv_obj_create(parent);
    lv_obj_remove_style_all(control_screen);
    lv_obj_add_style(control_screen, &cmain_style, 0);
    lv_obj_set_size(control_screen, lv_pct(100), lv_pct(100));
    lv_obj_center(control_screen);
    lv_obj_clear_flag(control_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(control_screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(control_screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(control_screen, 12, 0);
    lv_obj_add_event_cb(control_screen, screen_events, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(control_screen, screen_events, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(control_screen, control_screen_on_delete, LV_EVENT_DELETE, NULL);

    lv_obj_t* header = lv_obj_create(control_screen);
    lv_obj_remove_style_all(header);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(header, lv_pct(100), LV_SIZE_CONTENT);
    //lv_obj_set_style_pad_left(header, 20, 0);
    //lv_obj_set_style_pad_right(header, 20, 0);
    //lv_obj_set_style_pad_top(header, 10, 0);
    //lv_obj_set_style_pad_bottom(header, 6, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    time_label = lv_label_create(header);
    lv_obj_set_style_text_font(time_label, &font_bold_32, 0);
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_label_set_text(time_label, "--:--");

    lv_obj_t* grid = lv_obj_create(control_screen);
    lv_obj_remove_style_all(grid);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    //lv_obj_set_width(grid, lv_pct(100));
    lv_obj_set_size(grid, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_top(grid, 10, 0);
    lv_obj_set_style_pad_left(grid, 12, 0);
    lv_obj_set_style_pad_right(grid, 12, 0);
    lv_obj_set_style_pad_row(grid, 14, 0);
    lv_obj_set_style_pad_column(grid, 14, 0);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Create grid-like items (flex row wrap, non-scrollable) */
    for (uint32_t i = 0; i < 6; i++) {
        lv_obj_t* item = lv_obj_create(grid);
        lv_obj_remove_style_all(item);
        lv_obj_set_width(item, lv_pct(46));
        lv_obj_set_height(item, 110);
        lv_obj_set_style_bg_color(item, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_bg_opa(item, 38, 0);
        lv_obj_set_style_radius(item, 16, 0);
        lv_obj_set_style_pad_all(item, 8, 0);
        lv_obj_set_style_text_align(item, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_flex_flow(item, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(item, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        /* Make Silence and Bluetooth toggles */
        if (i == CTRL_SILENCE || i == CTRL_BLUETOOTH) {
            lv_obj_add_flag(item, LV_OBJ_FLAG_CHECKABLE);
            lv_obj_set_style_bg_color(item, lv_color_hex(0x438bff), LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_bg_opa(item, 255, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_add_event_cb(item, toggle_event_cb, LV_EVENT_VALUE_CHANGED, (void*)(uintptr_t)i);

            if (i == CTRL_SILENCE) {
                bool sound_on = settings_get_sound();
                bool silent = !sound_on;
                if (silent) {
                    lv_obj_add_state(item, LV_STATE_CHECKED);
                }
                else {
                    lv_obj_clear_state(item, LV_STATE_CHECKED);
                }
            }
            if (i == CTRL_BLUETOOTH) {
                bool ble_on = ble_sync_is_enabled();
                if (ble_on) {
                    lv_obj_add_state(item, LV_STATE_CHECKED);
                }
                else {
                    lv_obj_clear_state(item, LV_STATE_CHECKED);
                }
            }
        }
        else {
            lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(item, click_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        }

        lv_obj_t* image = lv_image_create(item);
        lv_image_set_src(image, control_icons[i]);
        lv_obj_set_align(image, LV_ALIGN_TOP_MID);
        lv_obj_remove_flag(image, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t* label = lv_label_create(item);
        lv_label_set_text(label, control_labels[i]);
        lv_obj_set_style_text_color(label, lv_color_hex(0xD0D0D0), 0);
        lv_obj_set_style_text_font(label, &font_normal_26, 0);
    }

    update_time_label();
    if (!time_timer) {
        time_timer = lv_timer_create(time_timer_cb, 1000, NULL);
        lv_timer_ready(time_timer);
    }
}

lv_obj_t* control_screen_get(void)
{
    if (control_screen == NULL) {
        // Create as a standalone screen if not yet created
        control_screen_create(NULL);
    }
    return control_screen;
}

static void click_event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    int ctrl = (int)(uintptr_t)lv_event_get_user_data(e);

    switch (ctrl) {
    case CTRL_BRIGHTNESS:
        ESP_LOGI(TAG, "Brightness control clicked");
        {
            lv_obj_t* t = ui_dynamic_tile_acquire();
            if (t) {
                // Create the content into the dynamic tile and switch to it
                lv_smartwatch_brightness_create(t);
                ui_dynamic_tile_show();
            }
        }
        break;
    case CTRL_BATTERY:
        ESP_LOGI(TAG, "Battery control clicked");
        {
            lv_obj_t* t = ui_dynamic_tile_acquire();
            if (t) {
                lv_smartwatch_batt_create(t);
                ui_dynamic_tile_show();
            }
        }
        break;
    case CTRL_FLASHLIGHT:
        ESP_LOGI(TAG, "Flashlight control clicked");
        //lv_indev_wait_release(lv_indev_active());
        {
            lv_obj_t* t = ui_dynamic_tile_acquire();
            if (t) {
                setting_flashlight_screen_create(t);
                ui_dynamic_tile_show();
            }
        }
        break;
    case CTRL_SETTINGS:
        ESP_LOGI(TAG, "Settings clicked");
         //lv_indev_wait_release(lv_indev_active());
        {
            lv_obj_t* t = ui_dynamic_tile_acquire();
            if (t) {
                settings_menu_screen_create(t);
                ui_dynamic_tile_show();
            }
        }
        break;
    default:
        break;
    }
}

static void toggle_event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    int ctrl = (int)(uintptr_t)lv_event_get_user_data(e);
    bool checked = lv_obj_has_state(obj, LV_STATE_CHECKED);

    switch (ctrl) {
    case CTRL_SILENCE:
        /* Silence ON means sound OFF */
        settings_set_sound(!checked);
        break;
    case CTRL_BLUETOOTH:
    {
        bool enable = checked;
        esp_err_t err = ble_sync_set_enabled(enable);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to %s Bluetooth: %s", enable ? "enable" : "disable", esp_err_to_name(err));
            if (enable) {
                lv_obj_clear_state(obj, LV_STATE_CHECKED);
            } else {
                lv_obj_add_state(obj, LV_STATE_CHECKED);
            }
            return;
        }
        settings_set_bluetooth_enabled(enable);
        break;
    }
    default:
        break;
    }
}

/* Scroll callback removed: screen is static and non-scrollable */
