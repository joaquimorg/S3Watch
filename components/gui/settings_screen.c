#include "settings_screen.h"
#include "settings.h"
#include "ui_fonts.h"
//#include "nimble-nordic-uart.h"


LV_IMAGE_DECLARE(image_mute_icon);
LV_IMAGE_DECLARE(image_flashlight_icon);
LV_IMAGE_DECLARE(image_brightness_icon);
LV_IMAGE_DECLARE(image_battery_icon);
LV_IMAGE_DECLARE(image_silence_icon);
LV_IMAGE_DECLARE(image_bluetooth_icon);
LV_IMAGE_DECLARE(image_settings_icon);

static void control_screen_events(lv_event_t* e);
static void toggle_event_cb(lv_event_t* e);

static lv_obj_t* control_screen;

static const lv_image_dsc_t* control_icons[] = {
    &image_brightness_icon,    
    &image_silence_icon,
    &image_flashlight_icon,
    &image_battery_icon,
    &image_bluetooth_icon,
    &image_settings_icon
};

static const char *control_labels[] = {
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

void lv_smartwatch_control_create(lv_obj_t* screen)
{

    static lv_style_t cmain_style;


    lv_style_init(&cmain_style);
    lv_style_set_text_color(&cmain_style, lv_color_white());
    lv_style_set_bg_color(&cmain_style, lv_color_hex(0x000000));
    lv_style_set_bg_opa(&cmain_style, LV_OPA_100);
    //lv_style_set_radius(&cmain_style, LV_RADIUS_CIRCLE);
    //lv_style_set_translate_y(&cmain_style, -SCREEN_SIZE);

    lv_style_set_layout(&cmain_style, LV_LAYOUT_FLEX);
    lv_style_set_flex_flow(&cmain_style, LV_FLEX_FLOW_ROW_WRAP);
    lv_style_set_flex_main_place(&cmain_style, LV_FLEX_ALIGN_START);
    lv_style_set_flex_cross_place(&cmain_style, LV_FLEX_ALIGN_CENTER);
    lv_style_set_flex_track_place(&cmain_style, LV_FLEX_ALIGN_START);
    lv_style_set_pad_row(&cmain_style, 16);
    lv_style_set_pad_column(&cmain_style, 16);
    lv_style_set_pad_top(&cmain_style, 25);


    control_screen = lv_obj_create(screen);
    lv_obj_remove_style_all(control_screen);
    lv_obj_add_style(control_screen, &cmain_style, 0);
    lv_obj_set_size(control_screen, lv_pct(100), lv_pct(100));
    lv_obj_remove_flag(control_screen, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_clear_flag(control_screen, LV_OBJ_FLAG_SCROLLABLE);


    /* Create grid-like items (flex row wrap, non-scrollable) */
    for (uint32_t i = 0; i < 6; i++) {
        lv_obj_t* item = lv_obj_create(control_screen);
        lv_obj_remove_style_all(item);
        lv_obj_set_width(item, lv_pct(48));
        lv_obj_set_height(item, 120);
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
                if (silent) lv_obj_add_state(item, LV_STATE_CHECKED);
                else lv_obj_clear_state(item, LV_STATE_CHECKED);
            }
            if (i == CTRL_BLUETOOTH) {
                /* Default to enabled (unchecked = off? we make checked=enabled for consistency) */
                lv_obj_add_state(item, LV_STATE_CHECKED);
            }
        }

        lv_obj_t* image = lv_image_create(item);
        lv_image_set_src(image, control_icons[i]);
        lv_obj_set_align(image, LV_ALIGN_TOP_MID);
        lv_obj_remove_flag(image, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t * label = lv_label_create(item);
        lv_label_set_text(label, control_labels[i]);
        lv_obj_set_style_text_color(label, lv_color_hex(0xD0D0D0), 0);
        lv_obj_set_style_text_font(label, &font_normal_26, 0);
    }

}


/**********************
 *   STATIC FUNCTIONS
 **********************/

static void control_screen_events(lv_event_t* e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if (event_code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());

        /*if(dir == LV_DIR_LEFT) {
            lv_smartwatch_animate_y(lv_demo_smartwatch_get_control_screen(), -SCREEN_SIZE, 1000, 0);
            //lv_smartwatch_animate_arc(arc_cont, ARC_EXPAND_UP, 700, 300);
            //lv_smartwatch_anim_opa(main_arc, 255, 500, 500);
        }*/

    }
    /*if(event_code == LV_EVENT_LONG_PRESSED) {
        lv_smartwatch_animate_y(lv_demo_smartwatch_get_control_screen(), -SCREEN_SIZE, 1000, 0);
        lv_smartwatch_animate_arc(arc_cont, ARC_EXPAND_UP, 700, 300);
        lv_smartwatch_anim_opa(main_arc, 255, 500, 500);
    }*/
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
        /* Simple enable/disable of BLE Nordic UART */
        /*if (checked) {
            nordic_uart_start("ESP32 S3 Watch", NULL);
        } else {
            nordic_uart_stop();
        }*/
        break;
    default:
        break;
    }
}

/* Scroll callback removed: screen is static and non-scrollable */
