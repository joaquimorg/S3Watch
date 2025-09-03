#include "setting_sound_screen.h"
#include "ui.h"
#include "ui_fonts.h"
#include "settings.h"
#include "audio_alert.h"
#include "settings_menu_screen.h"
#include "esp_log.h"

static lv_obj_t* ssound_screen;
static lv_obj_t* s_switch;
static lv_obj_t* s_slider;
static lv_obj_t* s_value_lbl;
static lv_obj_t* s_test_btn;
static void on_delete(lv_event_t* e);
static const char* TAG = "SoundSettings";

static void screen_events(lv_event_t* e)
{
    // Async back to avoid heavy work in event context    
    if (lv_event_get_code(e) == LV_EVENT_GESTURE) {
        if (lv_indev_get_gesture_dir(lv_indev_active()) == LV_DIR_RIGHT) {
            lv_indev_wait_release(lv_indev_active());
            ui_dynamic_subtile_close();
            ssound_screen = NULL;
        }
    }
}

static void toggle(lv_event_t* e)
{
    bool on = lv_obj_has_state(s_switch, LV_STATE_CHECKED);
    settings_set_sound(on);
    // Enable/disable slider accordingly
    if (s_slider) {
        if (on) lv_obj_clear_state(s_slider, LV_STATE_DISABLED);
        else lv_obj_add_state(s_slider, LV_STATE_DISABLED);
    }
    if (s_test_btn) {
        if (on) lv_obj_clear_state(s_test_btn, LV_STATE_DISABLED);
        else lv_obj_add_state(s_test_btn, LV_STATE_DISABLED);
    }
}

static void on_vol_change(lv_event_t* e)
{
    int32_t v = lv_slider_get_value(s_slider);
    settings_set_notify_volume((uint8_t)v);
    if (s_value_lbl) {
        char buf[8]; snprintf(buf, sizeof(buf), "%d%%", (int)v);
        lv_label_set_text(s_value_lbl, buf);
    }
}

static void test_btn_cb(lv_event_t* e)
{
    (void)e;
    audio_alert_notify();
}

void setting_sound_screen_create(lv_obj_t* parent)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_color(&style, lv_color_white());
    lv_style_set_bg_color(&style, lv_color_black());
    lv_style_set_bg_opa(&style, LV_OPA_COVER);

    ssound_screen = lv_obj_create(parent);
    lv_obj_remove_style_all(ssound_screen);
    lv_obj_add_style(ssound_screen, &style, 0);
    lv_obj_set_size(ssound_screen, lv_pct(100), lv_pct(100));
    // Allow gestures to bubble for tileview swipes
    lv_obj_add_flag(ssound_screen, LV_OBJ_FLAG_GESTURE_BUBBLE);
    // Mark as settings child for HW back button routing
    lv_obj_add_flag(ssound_screen, LV_OBJ_FLAG_USER_1);
    lv_obj_add_event_cb(ssound_screen, screen_events, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(ssound_screen, on_delete, LV_EVENT_DELETE, NULL);

    lv_obj_t* hdr = lv_obj_create(ssound_screen);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_size(hdr, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_t* title = lv_label_create(hdr);
    lv_obj_set_style_text_font(title, &font_bold_32, 0);
    lv_label_set_text(title, "Sound");

    lv_obj_t* content = lv_obj_create(ssound_screen);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, lv_pct(100), lv_pct(90));
    lv_obj_add_flag(content, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_style_pad_all(content, 16, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    s_switch = lv_switch_create(content);
    lv_obj_set_size(s_switch, 120, 50);
    if (settings_get_sound()) lv_obj_add_state(s_switch, LV_STATE_CHECKED);
    else lv_obj_clear_state(s_switch, LV_STATE_CHECKED);
    lv_obj_add_event_cb(s_switch, toggle, LV_EVENT_VALUE_CHANGED, NULL);

    // Volume row
    lv_obj_t* row = lv_obj_create(content);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(row, 8, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t* lbl = lv_label_create(row);
    lv_obj_set_style_text_font(lbl, &font_normal_28, 0);
    lv_label_set_text(lbl, "Notification Volume");

    s_value_lbl = lv_label_create(row);
    lv_obj_set_style_text_font(s_value_lbl, &font_bold_28, 0);
    char vbuf[8]; snprintf(vbuf, sizeof(vbuf), "%u%%", (unsigned)settings_get_notify_volume());
    lv_label_set_text(s_value_lbl, vbuf);

    s_slider = lv_slider_create(content);
    lv_obj_set_width(s_slider, lv_pct(100));
    lv_slider_set_range(s_slider, 0, 100);
    lv_slider_set_value(s_slider, settings_get_notify_volume(), LV_ANIM_OFF);
    lv_obj_add_event_cb(s_slider, on_vol_change, LV_EVENT_VALUE_CHANGED, NULL);
    if (!settings_get_sound()) lv_obj_add_state(s_slider, LV_STATE_DISABLED);

    // Test sound button
    s_test_btn = lv_btn_create(content);
    lv_obj_set_width(s_test_btn, lv_pct(100));
    lv_obj_t* test_lbl = lv_label_create(s_test_btn);
    lv_label_set_text(test_lbl, "Test Sound");
    lv_obj_center(test_lbl);
    lv_obj_add_event_cb(s_test_btn, test_btn_cb, LV_EVENT_CLICKED, NULL);
    if (!settings_get_sound()) lv_obj_add_state(s_test_btn, LV_STATE_DISABLED);
}

static void on_delete(lv_event_t* e)
{
    (void)e;
    ESP_LOGI(TAG, "Sound settings screen deleted");
    ssound_screen = NULL;
}

lv_obj_t* setting_sound_screen_get(void)
{
    if (!ssound_screen) {
        bsp_display_lock(0);
        setting_sound_screen_create(NULL);
        bsp_display_unlock();
    }
    return ssound_screen;
}
