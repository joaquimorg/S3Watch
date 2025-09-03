#include "setting_storage_screen.h"
#include "ui.h"
#include "ui_fonts.h"
#include "settings.h"
#include <string.h>
#include <stdio.h>
#include "lvgl.h"
#include "storage_file_explorer.h"
#include "settings_menu_screen.h"
#include "esp_log.h"

static lv_obj_t* sstorage_screen;
static void on_delete(lv_event_t* e);
static const char* TAG = "StorageSettings";
static void toast_timer_cb(lv_timer_t* t)
{
    lv_obj_t* obj = (lv_obj_t*)lv_timer_get_user_data(t);
    if (obj) lv_obj_del(obj);
}

static void show_toast(const char* text)
{
    if (!text) return;
    lv_obj_t* toast = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(toast);
    lv_obj_set_style_bg_color(toast, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(toast, LV_OPA_80, 0);
    lv_obj_set_style_radius(toast, 12, 0);
    lv_obj_set_style_pad_all(toast, 10, 0);
    lv_obj_set_style_border_width(toast, 0, 0);
    lv_obj_set_style_min_width(toast, 120, 0);
    lv_obj_set_style_max_width(toast, lv_pct(90), 0);

    lv_obj_t* lbl = lv_label_create(toast);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_center(lbl);

    lv_obj_update_layout(toast);
    lv_obj_align(toast, LV_ALIGN_BOTTOM_MID, 0, -20);

    (void)lv_timer_create(toast_timer_cb, 1200, toast);
}

static void screen_events(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_GESTURE) {
        if (lv_indev_get_gesture_dir(lv_indev_active()) == LV_DIR_RIGHT) {
            lv_indev_wait_release(lv_indev_active());
            ui_dynamic_subtile_close();
            sstorage_screen = NULL;
        }
    }
}

static void modal_btn_evt(lv_event_t* e)
{
    lv_obj_t* btn = lv_event_get_target(e);
    const char* action = (const char*)lv_event_get_user_data(e);
    const char* txt = lv_label_get_text(lv_obj_get_child(btn, 0));
    bool ok = txt && (strcmp(txt, "Restore") == 0 || strcmp(txt, "Format") == 0 || strcmp(txt, "OK") == 0);
    if (ok) {
        if (action && strcmp(action, "restore") == 0) {
            settings_reset_defaults();
            show_toast("Defaults restored");
        } else if (action && strcmp(action, "format") == 0) {
            settings_format_spiffs();
            show_toast("SPIFFS formatted");
        }
    }
    // Buttons live inside a row which lives inside the modal. Remove the modal.
    lv_obj_t* row = lv_obj_get_parent(btn);
    lv_obj_t* modal = row ? lv_obj_get_parent(row) : NULL;
    if (!modal) {
        // Fallback: if hierarchy changes, at least remove the immediate parent
        modal = row ? row : btn;
    }
    lv_obj_del(modal);
}

static void create_confirm_modal(const char* title, const char* text, const char* ok_label, const char* action)
{
    lv_obj_t* modal = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(modal);
    lv_obj_set_size(modal, lv_pct(90), LV_SIZE_CONTENT);
    lv_obj_center(modal);
    lv_obj_set_style_bg_color(modal, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(modal, LV_OPA_90, 0);
    lv_obj_set_style_radius(modal, 12, 0);
    lv_obj_set_style_pad_all(modal, 12, 0);
    lv_obj_set_flex_flow(modal, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(modal, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* lbl_t = lv_label_create(modal);
    lv_label_set_text(lbl_t, title);
    lv_obj_set_style_text_font(lbl_t, &font_bold_28, 0);
    lv_obj_set_style_text_color(lbl_t, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t* lbl = lv_label_create(modal);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &font_normal_26, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xE0E0E0), 0);

    lv_obj_t* row = lv_obj_create(modal);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* btn_cancel = lv_btn_create(row);
    lv_obj_t* lbl_c = lv_label_create(btn_cancel);
    lv_label_set_text(lbl_c, "Cancel");
    lv_obj_set_style_text_font(lbl_c, &font_normal_26, 0);
    lv_obj_add_event_cb(btn_cancel, modal_btn_evt, LV_EVENT_CLICKED, (void*)"cancel");

    lv_obj_t* btn_ok = lv_btn_create(row);
    lv_obj_t* lbl_ok = lv_label_create(btn_ok);
    lv_label_set_text(lbl_ok, ok_label);
    lv_obj_set_style_text_font(lbl_ok, &font_normal_26, 0);
    lv_obj_add_event_cb(btn_ok, modal_btn_evt, LV_EVENT_CLICKED, (void*)action);
}

static void confirm_restore(lv_event_t* e)
{
    (void)e;
    create_confirm_modal("Confirm", "Restore settings to defaults?\nThis will overwrite current settings.", "Restore", "restore");
}

static void confirm_format(lv_event_t* e)
{
    (void)e;
    create_confirm_modal("Warning", "Format SPIFFS?\nAll files will be erased.", "Format", "format");
}


static void show_spiffs_files(lv_event_t* e)
{
    (void)e;
    lv_indev_wait_release(lv_indev_active());
    // Open file explorer in the same second-level tile (replace content)
    lv_obj_t* tile = ui_dynamic_subtile_acquire();
    if (tile) {
        lv_obj_clean(tile);
        storage_file_explorer_screen_create(tile);
        ui_dynamic_subtile_show();
    }
}

void setting_storage_screen_create(lv_obj_t* parent)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_color(&style, lv_color_white());
    lv_style_set_bg_color(&style, lv_color_black());
    lv_style_set_bg_opa(&style, LV_OPA_COVER);

    sstorage_screen = lv_obj_create(parent);
    lv_obj_remove_style_all(sstorage_screen);
    lv_obj_add_style(sstorage_screen, &style, 0);
    lv_obj_set_size(sstorage_screen, lv_pct(100), lv_pct(100));
    lv_obj_add_event_cb(sstorage_screen, screen_events, LV_EVENT_GESTURE, NULL);
    // Allow gestures to bubble for tileview swipes
    lv_obj_add_flag(sstorage_screen, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(sstorage_screen, LV_OBJ_FLAG_USER_1);
    lv_obj_add_event_cb(sstorage_screen, on_delete, LV_EVENT_DELETE, NULL);

    lv_obj_t* hdr = lv_obj_create(sstorage_screen);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_size(hdr, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_t* title = lv_label_create(hdr);
    lv_obj_set_style_text_font(title, &font_bold_32, 0);
    lv_label_set_text(title, "Storage Tools");

    lv_obj_t* content = lv_obj_create(sstorage_screen);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, lv_pct(100), lv_pct(80));
    lv_obj_set_style_pad_top(content, 80, 0);
    lv_obj_set_style_pad_bottom(content, 10, 0);
    lv_obj_set_style_pad_left(content, 12, 0);
    lv_obj_set_style_pad_right(content, 12, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    lv_obj_t* btn_restore = lv_btn_create(content);
    lv_obj_set_size(btn_restore, lv_pct(100), 60);
    lv_obj_add_event_cb(btn_restore, confirm_restore, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_r = lv_label_create(btn_restore);
    lv_obj_set_style_text_font(lbl_r, &font_bold_28, 0);
    lv_label_set_text(lbl_r, "Restore Defaults");

    lv_obj_t* btn_format = lv_btn_create(content);
    lv_obj_set_size(btn_format, lv_pct(100), 60);
    lv_obj_add_event_cb(btn_format, confirm_format, LV_EVENT_CLICKED, NULL);
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

static void on_delete(lv_event_t* e)
{
    (void)e;
    ESP_LOGI(TAG, "Storage settings screen deleted");
    sstorage_screen = NULL;
}

lv_obj_t* setting_storage_screen_get(void)
{
    if (!sstorage_screen) {
        bsp_display_lock(0);
        setting_storage_screen_create(NULL);
        bsp_display_unlock();
    }
    return sstorage_screen;
}
