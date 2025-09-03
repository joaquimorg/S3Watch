#include "brightness_screen.h"
#include "ui_fonts.h"
#include "ui.h"
#include "settings.h"
#include "esp_log.h"
#include "bsp/esp32_s3_touch_amoled_2_06.h"
#include "settings_screen.h"

static const char* TAG = "BrightnessScreen";

LV_IMAGE_DECLARE(image_brightness_48);

static lv_obj_t* brightness_screen;
static lv_obj_t* percent_label;
static lv_obj_t* slider;

static void screen_events(lv_event_t* e);
static void on_delete(lv_event_t* e);
static void slider_event(lv_event_t* e);
static void update_label_from_value(int val);

void lv_smartwatch_brightness_create(lv_obj_t* screen)
{
    static lv_style_t cmain_style;
    static lv_style_t style_knob;

    lv_style_init(&cmain_style);
    lv_style_set_text_color(&cmain_style, lv_color_white());
    lv_style_set_bg_color(&cmain_style, lv_color_hex(0x000000));
    lv_style_set_bg_opa(&cmain_style, LV_OPA_100);

    lv_style_init(&style_knob);
    lv_style_set_bg_opa(&style_knob, LV_OPA_COVER);
    lv_style_set_bg_color(&style_knob, lv_color_hex(0xFFFF10));    
    lv_style_set_border_width(&style_knob, 0);
    lv_style_set_radius(&style_knob, LV_RADIUS_CIRCLE);
    lv_style_set_pad_all(&style_knob, 8); /*Makes the knob larger*/
    
    brightness_screen = lv_obj_create(screen);
    lv_obj_remove_style_all(brightness_screen);
    lv_obj_add_style(brightness_screen, &cmain_style, 0);
    lv_obj_set_size(brightness_screen, lv_pct(100), lv_pct(100));
    // Allow gestures on children to bubble up so tileview can handle swipes
    lv_obj_add_flag(brightness_screen, LV_OBJ_FLAG_GESTURE_BUBBLE);
    //lv_obj_clear_flag(brightness_screen, LV_OBJ_FLAG_SCROLLABLE);


    lv_obj_t* hdr_card = lv_obj_create(brightness_screen);
    lv_obj_remove_style_all(hdr_card);
    //lv_obj_set_size(hdr_card, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hdr_card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_obj_set_size(hdr_card, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_align(hdr_card, LV_ALIGN_TOP_MID);

    lv_obj_t* img = lv_image_create(hdr_card);
    lv_image_set_src(img, &image_brightness_48);

    // Big title label
    lv_obj_t* brightness_title_label = lv_label_create(hdr_card);
    lv_obj_set_style_text_font(brightness_title_label, &font_bold_32, 0);
    lv_label_set_text(brightness_title_label, "Brightness");
    //lv_obj_set_style_pad_bottom(brightness_title_label, 30, 0);
    //lv_obj_set_align(brightness_title_label, LV_ALIGN_TOP_MID);
    lv_obj_set_y(brightness_title_label, 10);

    // Layout
    //lv_obj_set_flex_flow(brightness_screen, LV_FLEX_FLOW_COLUMN);
    //lv_obj_set_flex_align(brightness_screen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    //lv_obj_set_style_pad_all(brightness_screen, 24, 0);
    //lv_obj_set_style_pad_row(brightness_screen, 18, 0);

    // Big percent label
    percent_label = lv_label_create(brightness_screen);
    lv_obj_set_style_text_font(percent_label, &font_bold_42, 0);
    lv_obj_set_style_text_color(percent_label, lv_color_hex(0xFFFF10), 0);
    lv_label_set_text(percent_label, "--%");
    lv_obj_set_align(percent_label, LV_ALIGN_CENTER);
    lv_obj_set_y(percent_label, 60);

    // Slider 0..100
    slider = lv_slider_create(brightness_screen);
    lv_obj_set_width(slider, lv_pct(90));
    lv_obj_set_height(slider, 30);
    lv_slider_set_range(slider, 5, 100);
    lv_obj_add_style(slider, &style_knob, LV_PART_KNOB);
    lv_obj_add_event_cb(slider, slider_event, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_align(slider, LV_ALIGN_CENTER);

    // Set initial value from settings
    int init = settings_get_brightness();
    if (init < 0) init = 100;
    if (init > 100) init = 100;
    lv_slider_set_value(slider, init, LV_ANIM_OFF);
    
    update_label_from_value(init);

    lv_obj_add_event_cb(brightness_screen, screen_events, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(brightness_screen, on_delete, LV_EVENT_DELETE, NULL);
}

lv_obj_t* brightness_screen_get(void)
{
    if (brightness_screen == NULL) {
        lv_smartwatch_brightness_create(NULL);
    }
    return brightness_screen;
}

static void update_label_from_value(int val)
{
    char txt[16];
    if (val < 0) {
        val = 0;
    }
    if (val > 100) {
        val = 100;
    }
    snprintf(txt, sizeof(txt), "%d%%", val);
    lv_label_set_text(percent_label, txt);
}

static void slider_event(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    int val = lv_slider_get_value(obj);
    update_label_from_value(val);
    settings_set_brightness(val);
}

static void screen_events(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
        if (dir == LV_DIR_RIGHT) {
            lv_indev_wait_release(lv_indev_active());
            // Navigate back to controls tile and destroy dynamic tile
            ui_dynamic_tile_close();
            // Local pointer becomes invalid after tile deletion; reset
            brightness_screen = NULL;
        }
    }
}

static void on_delete(lv_event_t* e)
{
    (void)e;
    ESP_LOGI(TAG, "Brightness screen deleted");
    brightness_screen = NULL;
}
