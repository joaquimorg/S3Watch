#include "setting_flashlight_screen.h"

#include "ui.h"
#include "ui_fonts.h"
#include "settings.h"
#include "settings_menu_screen.h"
#include "bsp/esp32_s3_touch_amoled_2_06.h"
#include "esp_log.h"

static lv_obj_t* s_flashlight_screen;
static int s_prev_brightness = -1;
static const char* TAG = "Flashlight";

static void apply_flashlight(void)
{
    if (s_prev_brightness < 0) {
        s_prev_brightness = settings_get_brightness();
    }
    if (bsp_display_lock(100)) {
        bsp_display_brightness_set(100);
        bsp_display_unlock();
    } else {
        bsp_display_brightness_set(100);
    }
}

static void restore_brightness(void)
{
    if (s_prev_brightness >= 0) {
        if (bsp_display_lock(100)) {
            bsp_display_brightness_set(s_prev_brightness);
            bsp_display_unlock();
        } else {
            bsp_display_brightness_set(s_prev_brightness);
        }
        s_prev_brightness = -1;
    }
}

static void screen_events(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_SCREEN_LOADED) {
        apply_flashlight();
    } else if (code == LV_EVENT_GESTURE) {
        if (lv_indev_get_gesture_dir(lv_indev_active()) == LV_DIR_RIGHT) {
            restore_brightness();
            lv_indev_wait_release(lv_indev_active());
            // Navigate back to controls tile and destroy dynamic tile
            ui_dynamic_tile_close();
            // Local pointer becomes invalid after tile deletion; reset
            s_flashlight_screen = NULL;
        }
    }
}

static void on_delete(lv_event_t* e)
{
    (void)e;
    ESP_LOGI(TAG, "Flashlight screen deleted");
    restore_brightness();
    s_flashlight_screen = NULL;

}

void setting_flashlight_screen_create(lv_obj_t* parent)
{
    s_flashlight_screen = lv_obj_create(parent);
    lv_obj_remove_style_all(s_flashlight_screen);
    lv_obj_set_size(s_flashlight_screen, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(s_flashlight_screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(s_flashlight_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_flashlight_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_flashlight_screen, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_event_cb(s_flashlight_screen, screen_events, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(s_flashlight_screen, on_delete, LV_EVENT_DELETE, NULL);

    lv_obj_t* label = lv_label_create(s_flashlight_screen);
    lv_obj_set_style_text_color(label, lv_color_black(), 0);
    lv_obj_set_style_text_font(label, &font_bold_32, 0);
    lv_label_set_text(label, "Swipe right to exit");
    lv_obj_center(label);
}

lv_obj_t* setting_flashlight_screen_get(void)
{
    if (!s_flashlight_screen) {
        if (bsp_display_lock(0)) {
            setting_flashlight_screen_create(NULL);
            bsp_display_unlock();
        } else {
            setting_flashlight_screen_create(NULL);
        }
    }
    return s_flashlight_screen;
}
