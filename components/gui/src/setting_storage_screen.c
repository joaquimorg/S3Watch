#include "setting_storage_screen.h"
#include "ui.h"
#include "ui_fonts.h"
#include "settings.h"
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

static lv_obj_t* s_screen;

static void screen_events(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_GESTURE) {
        if (lv_indev_get_gesture_dir(lv_indev_active()) == LV_DIR_RIGHT) {
            extern lv_obj_t* settings_menu_screen_get(void);
            load_screen(settings_menu_screen_get(), LV_SCR_LOAD_ANIM_MOVE_RIGHT);
        }
    }
}

static void restore_defaults(lv_event_t* e)
{
    (void)e; settings_reset_defaults();
}

static void format_spiffs(lv_event_t* e)
{
    (void)e; settings_format_spiffs();
}

static void close_modal_cb(lv_event_t* e)
{
    lv_obj_t* btn = lv_event_get_target(e);
    lv_obj_t* modal = lv_obj_get_parent(btn);
    if (modal) lv_obj_del(modal);
}

static void show_spiffs_files(lv_event_t* e)
{
    (void)e;
    lv_obj_t* modal = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(modal);
    lv_obj_set_size(modal, lv_pct(90), lv_pct(80));
    lv_obj_center(modal);
    lv_obj_set_style_radius(modal, 12, 0);
    lv_obj_set_style_bg_color(modal, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(modal, LV_OPA_90, 0);
    lv_obj_set_style_pad_all(modal, 8, 0);
    lv_obj_set_flex_flow(modal, LV_FLEX_FLOW_COLUMN);

    lv_obj_t* title = lv_label_create(modal);
    lv_obj_set_style_text_font(title, &font_bold_32, 0);
    lv_label_set_text(title, "SPIFFS Files");

    lv_obj_t* area = lv_textarea_create(modal);
    lv_textarea_set_one_line(area, false);
    lv_obj_set_size(area, lv_pct(100), lv_pct(70));
    lv_textarea_set_text(area, "");

    lv_obj_t* btn_close = lv_btn_create(modal);
    lv_obj_t* lbl_close = lv_label_create(btn_close);
    lv_label_set_text(lbl_close, "Close");
    lv_obj_add_event_cb(btn_close, close_modal_cb, LV_EVENT_CLICKED, NULL);

    char buf[1024]; size_t off = 0;
    DIR* dir = opendir("/spiffs");
    if (!dir) { snprintf(buf, sizeof(buf), "Cannot open /spiffs\n"); lv_textarea_set_text(area, buf); return; }
    struct dirent* de; struct stat st;
    while ((de = readdir(dir)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        char path[256]; snprintf(path, sizeof(path), "/spiffs/%s", de->d_name);
        long sz = 0; if (stat(path, &st) == 0) sz = (long)st.st_size;
        int n = snprintf(buf + off, sizeof(buf) - off, "%s\t%ld bytes\n", de->d_name, sz);
        if (n < 0 || (size_t)n >= sizeof(buf) - off) {
            break;
        }
        off += (size_t)n;
    }
    closedir(dir);
    if (off == 0) snprintf(buf, sizeof(buf), "<empty>\n");
    lv_textarea_set_text(area, buf);
}

void setting_storage_screen_create(lv_obj_t* parent)
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
    lv_label_set_text(title, "Storage Tools");

    lv_obj_t* content = lv_obj_create(s_screen);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, lv_pct(100), lv_pct(100));
    lv_obj_set_style_pad_all(content, 16, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    lv_obj_t* btn_restore = lv_btn_create(content);
    lv_obj_set_size(btn_restore, lv_pct(100), 60);
    lv_obj_add_event_cb(btn_restore, restore_defaults, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_r = lv_label_create(btn_restore);
    lv_obj_set_style_text_font(lbl_r, &font_bold_28, 0);
    lv_label_set_text(lbl_r, "Restore Defaults");

    lv_obj_t* btn_format = lv_btn_create(content);
    lv_obj_set_size(btn_format, lv_pct(100), 60);
    lv_obj_add_event_cb(btn_format, format_spiffs, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_f = lv_label_create(btn_format);
    lv_obj_set_style_text_font(lbl_f, &font_bold_28, 0);
    lv_label_set_text(lbl_f, "Format SPIFFS");

    lv_obj_t* btn_list = lv_btn_create(content);
    lv_obj_set_size(btn_list, lv_pct(100), 60);
    lv_obj_add_event_cb(btn_list, show_spiffs_files, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_l = lv_label_create(btn_list);
    lv_obj_set_style_text_font(lbl_l, &font_bold_28, 0);
    lv_label_set_text(lbl_l, "View Files");
}

lv_obj_t* setting_storage_screen_get(void)
{
    if (!s_screen) setting_storage_screen_create(NULL);
    return s_screen;
}
