#include "setting_sound_screen.h"
#include "ui.h"
#include "ui_fonts.h"
#include "settings.h"

static lv_obj_t* s_screen;
static lv_obj_t* s_switch;

static void screen_events(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_GESTURE) {
        if (lv_indev_get_gesture_dir(lv_indev_active()) == LV_DIR_RIGHT) {
            extern lv_obj_t* settings_menu_screen_get(void);
            load_screen(settings_menu_screen_get(), LV_SCR_LOAD_ANIM_MOVE_RIGHT);
        }
    }
}

static void toggle(lv_event_t* e)
{
    bool on = lv_obj_has_state(s_switch, LV_STATE_CHECKED);
    settings_set_sound(on);
}

void setting_sound_screen_create(lv_obj_t* parent)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_color(&style, lv_color_white());
    lv_style_set_bg_color(&style, lv_color_black());
    lv_style_set_bg_opa(&style, LV_OPA_COVER);

    s_screen = lv_obj_create(parent);
    lv_obj_remove_style_all(s_screen);
    lv_obj_add_style(s_screen, &style, 0);
    lv_obj_set_size(s_screen, lv_pct(100), lv_pct(100));
    lv_obj_add_event_cb(s_screen, screen_events, LV_EVENT_ALL, NULL);

    lv_obj_t* hdr = lv_obj_create(s_screen);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_size(hdr, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_t* title = lv_label_create(hdr);
    lv_obj_set_style_text_font(title, &font_bold_32, 0);
    lv_label_set_text(title, "Sound");

    lv_obj_t* content = lv_obj_create(s_screen);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, lv_pct(100), lv_pct(90));
    lv_obj_set_style_pad_all(content, 16, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    s_switch = lv_switch_create(content);
    lv_obj_set_size(s_switch, 120, 50);
    if (settings_get_sound()) lv_obj_add_state(s_switch, LV_STATE_CHECKED);
    else lv_obj_clear_state(s_switch, LV_STATE_CHECKED);
    lv_obj_add_event_cb(s_switch, toggle, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t* lbl = lv_label_create(content);
    lv_obj_set_style_text_font(lbl, &font_normal_28, 0);
    lv_label_set_text(lbl, "Tap switch to toggle sound");
}

lv_obj_t* setting_sound_screen_get(void)
{
    if (!s_screen) setting_sound_screen_create(NULL);
    return s_screen;
}

