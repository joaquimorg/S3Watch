#include "setting_timeout_screen.h"
#include "ui.h"
#include "ui_fonts.h"
#include "settings.h"
#include "esp_log.h"
#include "settings_menu_screen.h"

static lv_obj_t* stimeout_screen;
static lv_obj_t* stimeout_content;
static void on_delete(lv_event_t* e);
static const char* TAG = "DisplayTimeout";

static void refresh_checked(void)
{
    if (!stimeout_content) return;
    uint32_t cur = settings_get_display_timeout();
    lv_obj_t* selected = NULL;
    for (uint32_t i = 0; i < lv_obj_get_child_count(stimeout_content); ++i) {
        lv_obj_t* row = lv_obj_get_child(stimeout_content, i);
        uint32_t val = (uint32_t)(uintptr_t)lv_obj_get_user_data(row);
        if (val == cur) {
            lv_obj_add_state(row, LV_STATE_CHECKED);
            selected = row;
        } else {
            lv_obj_remove_state(row, LV_STATE_CHECKED);
        }
    }
    if (selected) {
        lv_obj_scroll_to_view(selected, LV_ANIM_ON);
    }
}

static void screen_events(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_GESTURE) {
        if (lv_indev_get_gesture_dir(lv_indev_active()) == LV_DIR_RIGHT) {
            lv_indev_wait_release(lv_indev_active());
            ui_dynamic_subtile_close();
            stimeout_screen = NULL;
        }
    }
    else if (lv_event_get_code(e) == LV_EVENT_SCREEN_LOADED) {
        refresh_checked();
    }
}

static void choose(lv_event_t* e)
{
    uint32_t to = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    settings_set_display_timeout(to);
    // Update styles
    lv_obj_t* parent = lv_obj_get_parent(lv_event_get_target(e));
    uint32_t current = settings_get_display_timeout();
    for (uint32_t i = 0; i < lv_obj_get_child_count(parent); ++i) {
        lv_obj_t* row = lv_obj_get_child(parent, i);
        uint32_t val = (uint32_t)(uintptr_t)lv_obj_get_user_data(row);
        lv_obj_remove_state(row, LV_STATE_CHECKED);
        if (val == current) lv_obj_add_state(row, LV_STATE_CHECKED);
    }
}

static lv_obj_t* make_opt(lv_obj_t* parent, const char* txt, uint32_t val)
{
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 60);
    lv_obj_set_style_bg_opa(row, LV_OPA_10, 0);
    lv_obj_set_style_bg_color(row, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(row, 12, 0);
    lv_obj_set_style_pad_all(row, 12, 0);
    lv_obj_set_style_margin_bottom(row, 10, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, choose, LV_EVENT_CLICKED, (void*)(uintptr_t)val);
    lv_obj_set_user_data(row, (void*)(uintptr_t)val);
    lv_obj_t* l = lv_label_create(row);
    lv_obj_set_style_text_font(l, &font_bold_32, 0);
    lv_label_set_text(l, txt);
    return row;
}

void setting_timeout_screen_create(lv_obj_t* parent)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_color(&style, lv_color_white());
    lv_style_set_bg_color(&style, lv_color_black());
    lv_style_set_bg_opa(&style, LV_OPA_COVER);

    stimeout_screen = lv_obj_create(parent);
    lv_obj_remove_style_all(stimeout_screen);
    lv_obj_add_style(stimeout_screen, &style, 0);
    lv_obj_set_size(stimeout_screen, lv_pct(100), lv_pct(100));
    lv_obj_add_event_cb(stimeout_screen, screen_events, LV_EVENT_GESTURE, NULL);
    // Allow gestures to bubble for tileview swipes
    lv_obj_add_flag(stimeout_screen, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(stimeout_screen, LV_OBJ_FLAG_USER_1);
    lv_obj_add_event_cb(stimeout_screen, on_delete, LV_EVENT_DELETE, NULL);

    lv_obj_t* hdr = lv_obj_create(stimeout_screen);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_size(hdr, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_t* title = lv_label_create(hdr);
    lv_obj_set_style_text_font(title, &font_bold_32, 0);
    lv_label_set_text(title, "Display Timeout");

    stimeout_content = lv_obj_create(stimeout_screen);
    lv_obj_remove_style_all(stimeout_content);
    lv_obj_set_size(stimeout_content, lv_pct(100), lv_pct(80));
    lv_obj_add_flag(stimeout_content, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_style_pad_top(stimeout_content, 80, 0);
    lv_obj_set_style_pad_bottom(stimeout_content, 10, 0);
    lv_obj_set_style_pad_left(stimeout_content, 12, 0);
    lv_obj_set_style_pad_right(stimeout_content, 12, 0);
    lv_obj_set_flex_flow(stimeout_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(stimeout_content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_scroll_dir(stimeout_content, LV_DIR_VER);

    lv_obj_t* r10 = make_opt(stimeout_content, "10 s", SETTINGS_DISPLAY_TIMEOUT_10S);
    lv_obj_t* r20 = make_opt(stimeout_content, "20 s", SETTINGS_DISPLAY_TIMEOUT_20S);
    lv_obj_t* r30 = make_opt(stimeout_content, "30 s", SETTINGS_DISPLAY_TIMEOUT_30S);
    lv_obj_t* r60 = make_opt(stimeout_content, "1 min", SETTINGS_DISPLAY_TIMEOUT_1MIN);

    // Style for selected option for clear visibility
    for (uint32_t i = 0; i < lv_obj_get_child_count(stimeout_content); ++i) {
        lv_obj_t* row = lv_obj_get_child(stimeout_content, i);
        lv_obj_set_style_bg_color(row, lv_color_hex(0x438bff), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(row, 255, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(row, lv_color_white(), LV_PART_MAIN | LV_STATE_CHECKED);
    }

    refresh_checked();
}

static void on_delete(lv_event_t* e)
{
    (void)e;
    ESP_LOGI(TAG, "Display timeout screen deleted");
    stimeout_screen = NULL;
}

lv_obj_t* setting_timeout_screen_get(void)
{
    if (!stimeout_screen) {
        bsp_display_lock(0);
        setting_timeout_screen_create(NULL);
        bsp_display_unlock();
    }
    return stimeout_screen;
}
