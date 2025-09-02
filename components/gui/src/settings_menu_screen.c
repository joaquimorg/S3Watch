#include "settings_menu_screen.h"
#include "ui.h"
#include "ui_fonts.h"
#include "settings.h"
#include "setting_step_goal_screen.h"
#include "setting_timeout_screen.h"
#include "setting_sound_screen.h"
#include "setting_storage_screen.h"

#include "settings_screen.h"

static lv_obj_t* smenu_screen;
static lv_obj_t* smenu_content;
static lv_obj_t* r1;
static lv_obj_t* r2;
static lv_obj_t* r3;
static lv_obj_t* r4;

static void open_goal(lv_event_t* e){ (void)e; load_screen(NULL, setting_step_goal_screen_get(), LV_SCR_LOAD_ANIM_MOVE_LEFT); }
static void open_timeout(lv_event_t* e){ (void)e; load_screen(NULL, setting_timeout_screen_get(), LV_SCR_LOAD_ANIM_MOVE_LEFT); }
static void open_sound(lv_event_t* e){ (void)e; load_screen(NULL, setting_sound_screen_get(), LV_SCR_LOAD_ANIM_MOVE_LEFT); }
static void open_storage(lv_event_t* e){ (void)e; load_screen(NULL, setting_storage_screen_get(), LV_SCR_LOAD_ANIM_MOVE_LEFT); }
static void back_to_main(lv_event_t* e){ (void)e; lv_indev_wait_release(lv_indev_active()); load_screen(smenu_screen, get_main_screen(), LV_SCR_LOAD_ANIM_MOVE_RIGHT); }
static void refresh_values(lv_obj_t* content)
{
    if (!content) return;
    uint32_t goal = settings_get_step_goal();
    uint32_t to = settings_get_display_timeout();
    bool snd = settings_get_sound();
    char buf[16];
    
    snprintf(buf, sizeof(buf), "%u", (unsigned)goal);
    lv_label_set_text(r1, buf);
    const char* ttxt = (to==SETTINGS_DISPLAY_TIMEOUT_10S)?"10 s":(to==SETTINGS_DISPLAY_TIMEOUT_20S)?"20 s":(to==SETTINGS_DISPLAY_TIMEOUT_30S)?"30 s":"1 min";
    lv_label_set_text(r2, ttxt);
    if (snd) {
        char sbuf[16]; snprintf(sbuf, sizeof(sbuf), "On (%u%%)", (unsigned)settings_get_notify_volume());
        lv_label_set_text(r3, sbuf);
    } else {
        lv_label_set_text(r3, "Off");
    }
}

static void screen_events(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_GESTURE) {
        if (lv_indev_get_gesture_dir(lv_indev_active()) == LV_DIR_RIGHT) {
            lv_indev_wait_release(lv_indev_active());
            load_screen(smenu_screen, get_main_screen(), LV_SCR_LOAD_ANIM_MOVE_RIGHT);
        }
    }
    else if (lv_event_get_code(e) == LV_EVENT_SCREEN_LOADED) {
        refresh_values(smenu_content);
    }
}

static lv_obj_t* make_row(lv_obj_t* parent, const char* label_txt, const char* value_txt, lv_event_cb_t cb)
{
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 56);
    lv_obj_set_style_bg_opa(row, LV_OPA_10, 0);
    lv_obj_set_style_bg_color(row, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(row, 12, 0);
    lv_obj_set_style_pad_hor(row, 12, 0);
    lv_obj_set_style_pad_ver(row, 6, 0);
    lv_obj_set_style_margin_bottom(row, 8, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    if (cb) lv_obj_add_event_cb(row, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* l = lv_label_create(row);
    lv_obj_set_style_text_font(l, &font_normal_28, 0);
    lv_label_set_text(l, label_txt);
    lv_obj_t* v = lv_label_create(row);
    lv_obj_set_style_text_font(v, &font_bold_28, 0);
    lv_label_set_text(v, value_txt);
    return v;
}

/* duplicate refresh_values removed */

void settings_menu_screen_create(lv_obj_t* parent)
{
    static lv_style_t cmain_style;
    lv_style_init(&cmain_style);
    lv_style_set_text_color(&cmain_style, lv_color_white());
    lv_style_set_bg_color(&cmain_style, lv_color_black());
    lv_style_set_bg_opa(&cmain_style, LV_OPA_COVER);

    smenu_screen = lv_obj_create(parent);
    lv_obj_remove_style_all(smenu_screen);
    lv_obj_add_style(smenu_screen, &cmain_style, 0);
    lv_obj_set_size(smenu_screen, lv_pct(100), lv_pct(100));
    //lv_obj_add_flag(smenu_screen, LV_OBJ_FLAG_GESTURE_BUBBLE);    

    // Header
    /*lv_obj_t* hdr = lv_obj_create(smenu_screen);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_size(hdr, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);*/

    lv_obj_t* title = lv_label_create(smenu_screen);
    lv_obj_set_style_text_font(title, &font_bold_32, 0);
    lv_label_set_text(title, "Settings");
    //lv_obj_set_style_pad_top(title, 10, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Back button in the top-left corner: "<"
    lv_obj_t* back_btn = lv_btn_create(smenu_screen);
    lv_obj_remove_style_all(back_btn);
    lv_obj_set_size(back_btn, 48, 48);
    //lv_obj_set_style_bg_opa(back_btn, LV_OPA_20, 0);
    //lv_obj_set_style_bg_color(back_btn, lv_color_white(), 0);
    lv_obj_set_style_radius(back_btn, 8, 0);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 40, 6);
    lv_obj_add_event_cb(back_btn, back_to_main, LV_EVENT_CLICKED, NULL);
    lv_obj_t* back_lbl = lv_label_create(back_btn);
    lv_obj_set_style_text_font(back_lbl, &font_bold_32, 0);
    lv_label_set_text(back_lbl, "<");
    lv_obj_center(back_lbl);

    // Content list
    smenu_content = lv_obj_create(smenu_screen);
    lv_obj_add_flag(smenu_content, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_remove_style_all(smenu_content);
    lv_obj_set_size(smenu_content, lv_pct(100), lv_pct(85));
    lv_obj_center(smenu_content);
    //lv_obj_set_style_pad_top(smenu_content, 80, 0);
    //lv_obj_set_style_pad_bottom(smenu_content, 10, 0);
    lv_obj_set_style_pad_left(smenu_content, 12, 0);
    lv_obj_set_style_pad_right(smenu_content, 12, 0);

    lv_obj_set_flex_flow(smenu_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(smenu_content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    r1 = make_row(smenu_content, "Step Goal", "--", open_goal);
    r2 = make_row(smenu_content, "Display Timeout", "--", open_timeout);
    r3 = make_row(smenu_content, "Sound", "--", open_sound);
    r4 = make_row(smenu_content, "Storage", "Tools", open_storage);

    //refresh_values(smenu_content);
    lv_obj_add_event_cb(smenu_screen, screen_events, LV_EVENT_ALL, NULL);
}

lv_obj_t* settings_menu_screen_get(void)
{
    if (smenu_screen == NULL) {
        bsp_display_lock(0);
        settings_menu_screen_create(NULL);
        bsp_display_unlock();
    }
    return smenu_screen;
}
