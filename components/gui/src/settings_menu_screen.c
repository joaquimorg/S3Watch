#include "settings_menu_screen.h"
#include "ui.h"
#include "ui_fonts.h"
#include "settings.h"
#include "setting_step_goal_screen.h"
#include "setting_timeout_screen.h"
#include "setting_sound_screen.h"
#include "setting_storage_screen.h"

#include "settings_screen.h"
#include "esp_log.h"

static const char* TAG = "SettingsMenu";
static lv_obj_t* smenu_screen;
static void on_delete(lv_event_t* e);
static lv_obj_t* smenu_content;
static lv_obj_t* r1;
static lv_obj_t* r2;
static lv_obj_t* r3;
static lv_obj_t* r4;

static void open_goal(lv_event_t* e){ (void)e; lv_indev_wait_release(lv_indev_active()); lv_obj_t* t = ui_dynamic_subtile_acquire(); if (t){ setting_step_goal_screen_create(t); ui_dynamic_subtile_show(); } }
static void open_timeout(lv_event_t* e){ (void)e; lv_indev_wait_release(lv_indev_active()); lv_obj_t* t = ui_dynamic_subtile_acquire(); if (t){ setting_timeout_screen_create(t); ui_dynamic_subtile_show(); } }
static void open_sound(lv_event_t* e){ (void)e; lv_indev_wait_release(lv_indev_active()); lv_obj_t* t = ui_dynamic_subtile_acquire(); if (t){ setting_sound_screen_create(t); ui_dynamic_subtile_show(); } }
static void open_storage(lv_event_t* e){ (void)e; lv_indev_wait_release(lv_indev_active()); lv_obj_t* t = ui_dynamic_subtile_acquire(); if (t){ setting_storage_screen_create(t); ui_dynamic_subtile_show(); } }
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
            ui_dynamic_tile_close();
            smenu_screen = NULL;
        }
    }
    /*else if (lv_event_get_code(e) == LV_EVENT_SCREEN_LOADED) {
        refresh_values(smenu_content);
    }*/
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

typedef enum {
    LV_MENU_ITEM_BUILDER_VARIANT_1,
    LV_MENU_ITEM_BUILDER_VARIANT_2
} lv_menu_builder_variant_t;


static lv_obj_t * root_page;
static lv_obj_t * create_text(lv_obj_t * parent, const char * icon, const char * txt,
                              lv_menu_builder_variant_t builder_variant);
static lv_obj_t * create_slider(lv_obj_t * parent,
                                const char * icon, const char * txt, int32_t min, int32_t max, int32_t val);
static lv_obj_t * create_switch(lv_obj_t * parent,
                                const char * icon, const char * txt, bool chk);


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
    // Allow gestures to bubble so tileview can catch swipes
    lv_obj_add_flag(smenu_screen, LV_OBJ_FLAG_GESTURE_BUBBLE);

    // Header

    lv_obj_t* title = lv_label_create(smenu_screen);
    lv_obj_set_style_text_font(title, &font_bold_32, 0);
    lv_label_set_text(title, "Settings");
    //lv_obj_set_style_pad_top(title, 10, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t * menu = lv_menu_create(smenu_screen);

    lv_menu_set_mode_root_back_button(menu, LV_MENU_ROOT_BACK_BUTTON_DISABLED);
    lv_obj_set_size(menu, lv_pct(100), lv_pct(85));
    //lv_obj_set_style_pad_top(menu, 90, 0);
    lv_obj_add_style(menu, &cmain_style, 0);
    //lv_obj_center(menu);
    lv_obj_align(menu, LV_ALIGN_TOP_MID, 0, 50);

    lv_obj_t * cont;
    lv_obj_t * section;

    lv_obj_t * sub_sound_page = lv_menu_page_create(menu, "Sound");
    lv_obj_set_style_pad_hor(sub_sound_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_menu_separator_create(sub_sound_page);
    section = lv_menu_section_create(sub_sound_page);
    create_switch(section, LV_SYMBOL_AUDIO, "Sound", false);

    lv_obj_t * sub_display_page = lv_menu_page_create(menu, "Brightness");
    lv_obj_set_style_pad_hor(sub_display_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_menu_separator_create(sub_display_page);
    section = lv_menu_section_create(sub_display_page);
    create_slider(section, LV_SYMBOL_SETTINGS, "Select Brightness", 0, 150, 100);

    lv_obj_t * sub_menu_mode_page = lv_menu_page_create(menu, "Step Goal");
    (void)setting_step_goal_screen_get(sub_menu_mode_page);
    /*lv_obj_set_style_pad_hor(sub_menu_mode_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_menu_separator_create(sub_menu_mode_page);
    section = lv_menu_section_create(sub_menu_mode_page);
    cont = create_switch(section, LV_SYMBOL_AUDIO, "Sidebar enable", true);*/

    root_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(root_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    section = lv_menu_section_create(root_page);

    cont = create_text(section, LV_SYMBOL_AUDIO, "Sound", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_sound_page);

    cont = create_text(section, LV_SYMBOL_SETTINGS, "Display", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_display_page);

    cont = create_text(section, LV_SYMBOL_PLAY, "Step Goal", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_menu_mode_page);

    cont = create_text(section, LV_SYMBOL_POWER, "Display Timeout", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_menu_mode_page);

    cont = create_text(section, LV_SYMBOL_SAVE, "Storage", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_menu_mode_page);

    lv_menu_set_sidebar_page(menu, NULL);
    lv_menu_set_page(menu, root_page);
    /*

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

    refresh_values(smenu_content);*/

    lv_obj_add_event_cb(smenu_screen, screen_events, LV_EVENT_ALL, NULL);
    // Clear stale pointer if the tile is deleted externally
    lv_obj_add_event_cb(smenu_screen, on_delete, LV_EVENT_DELETE, NULL);
}

static lv_obj_t * create_text(lv_obj_t * parent, const char * icon, const char * txt,
                              lv_menu_builder_variant_t builder_variant)
{
    lv_obj_t * obj = lv_menu_cont_create(parent);

    lv_obj_t * img = NULL;
    lv_obj_t * label = NULL;

    if(icon) {
        img = lv_image_create(obj);
        lv_image_set_src(img, icon);
    }

    if(txt) {
        label = lv_label_create(obj);
        lv_label_set_text(label, txt);
        lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
        lv_obj_set_flex_grow(label, 1);
        lv_obj_set_style_text_font(label, &font_bold_26, 0);
    }

    if(builder_variant == LV_MENU_ITEM_BUILDER_VARIANT_2 && icon && txt) {
        lv_obj_add_flag(img, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
        lv_obj_swap(img, label);
    }

    return obj;
}

static lv_obj_t * create_slider(lv_obj_t * parent, const char * icon, const char * txt, int32_t min, int32_t max,
                                int32_t val)
{
    lv_obj_t * obj = create_text(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_2);

    lv_obj_t * slider = lv_slider_create(obj);
    lv_obj_set_flex_grow(slider, 1);
    lv_slider_set_range(slider, min, max);
    lv_slider_set_value(slider, val, LV_ANIM_OFF);

    if(icon == NULL) {
        lv_obj_add_flag(slider, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    }

    return obj;
}

static lv_obj_t * create_switch(lv_obj_t * parent, const char * icon, const char * txt, bool chk)
{
    lv_obj_t * obj = create_text(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_1);

    lv_obj_t * sw = lv_switch_create(obj);
    lv_obj_add_state(sw, chk ? LV_STATE_CHECKED : 0);

    return obj;
}


static void on_delete(lv_event_t* e)
{
    (void)e;
    ESP_LOGI(TAG, "Settings menu screen deleted");
    smenu_screen = NULL;
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
