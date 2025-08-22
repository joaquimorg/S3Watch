#include "watchface.h"
#include "sensors.h"
#include "settings_screen.h"
#include "ui_fonts.h"
#include "rtc_lib.h"


static lv_obj_t * home_screen;
static lv_obj_t * label_hour;
static lv_obj_t * label_minute;
static lv_obj_t * label_second;
static lv_obj_t * label_date;
static lv_obj_t * label_weekday;
static lv_obj_t * img_battery;
static lv_obj_t * lbl_batt_pct;
static lv_obj_t * lbl_charge_icon;
static lv_obj_t * img_ble;


//static lv_style_t main_style;


/*
static void home_screen_events(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if(event_code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
        if(dir == LV_DIR_BOTTOM) {
            lv_smartwatch_animate_y(lv_demo_smartwatch_get_control_screen(), 0, 800, 200);
            //lv_smartwatch_animate_arc(arc_cont, ARC_SHRINK_DOWN, 1000, 0);
            //lv_smartwatch_anim_opa(main_arc, 0, 700, 0);

        }
        if(dir == LV_DIR_LEFT) {
            lv_smartwatch_animate_x_from(lv_demo_smartwatch_get_health_screen(), SCREEN_SIZE, 0, 800, 200);
            //lv_smartwatch_animate_arc(arc_cont, ARC_SHRINK_LEFT, 1000, 0);
            //lv_smartwatch_anim_opa(main_arc, 0, 700, 0);
        }
    }
}
*/

static void update_time_task(lv_timer_t * timer)
{
    lv_label_set_text_fmt(label_hour, "%02d", rtc_get_hour());
    lv_label_set_text_fmt(label_minute, "%02d", rtc_get_minute());
    lv_label_set_text_fmt(label_second, "%02d", rtc_get_second());
    lv_label_set_text_fmt(label_date, "%02d/%02d", rtc_get_day(), rtc_get_month());
    lv_label_set_text(label_weekday, rtc_get_weekday_short_string());
}

void watchface_create(lv_obj_t * screen) {

    //lv_obj_remove_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);
    //lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), 0);

    /*if(!inited) {
        lv_style_init(&main_style);
        lv_style_set_text_color(&main_style, lv_color_white());
        lv_style_set_bg_color(&main_style, lv_color_black());
        lv_style_set_bg_opa(&main_style, LV_OPA_100);
        //lv_style_set_radius(&main_style, LV_RADIUS_CIRCLE);     
    }*/

    home_screen = lv_obj_create(screen);
    lv_obj_remove_style_all(home_screen);
    lv_obj_set_size(home_screen, lv_pct(100), lv_pct(100));
    //lv_obj_add_style(home_screen, &main_style, 0);
    lv_obj_remove_flag(home_screen, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_remove_flag(home_screen, LV_OBJ_FLAG_SCROLLABLE);

    LV_IMAGE_DECLARE(background_wf);
    lv_obj_t * image = lv_image_create(home_screen);
    lv_image_set_src(image, &background_wf);
    lv_obj_set_align(image, LV_ALIGN_CENTER);

    //lv_obj_add_event_cb(home_screen, home_screen_events, LV_EVENT_ALL, NULL);

    label_hour = lv_label_create(home_screen);
    lv_obj_set_y(label_hour, -95);
    lv_obj_set_align(label_hour, LV_ALIGN_CENTER);
    lv_label_set_text(label_hour, "--");
    lv_obj_set_style_text_letter_space(label_hour, 1, 0);
    lv_obj_set_style_text_font(label_hour, &font_numbers_160, 0);
    lv_obj_set_style_text_color(label_hour, lv_color_hex(0xF0B000), LV_PART_MAIN | LV_STATE_DEFAULT);

    label_minute = lv_label_create(home_screen);
    lv_obj_set_y(label_minute, 105);
    lv_obj_set_align(label_minute, LV_ALIGN_CENTER);
    lv_label_set_text(label_minute, "--");
    lv_obj_set_style_text_letter_space(label_minute, 1, 0);
    lv_obj_set_style_text_font(label_minute, &font_numbers_160, 0);
    lv_obj_set_style_text_color(label_minute, lv_color_hex(0x90F090), LV_PART_MAIN | LV_STATE_DEFAULT);

    label_second = lv_label_create(home_screen);
    lv_obj_set_align(label_second, LV_ALIGN_CENTER);
    lv_label_set_text(label_second, "--");
    lv_obj_set_style_text_letter_space(label_second, 1, 0);
    lv_obj_set_style_text_font(label_second, &font_numbers_80, 0);
    lv_obj_set_style_text_color(label_second, lv_color_hex(0x909090), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * date_cont = lv_obj_create(home_screen);
    lv_obj_remove_style_all(date_cont);
    lv_obj_set_size(date_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_x(date_cont, -20);
    lv_obj_set_align(date_cont, LV_ALIGN_RIGHT_MID);
    lv_obj_set_flex_flow(date_cont, LV_FLEX_FLOW_COLUMN);

    
    label_date = lv_label_create(date_cont);
    lv_label_set_text(label_date, "--/--");
    lv_obj_set_style_text_letter_space(label_date, 1, 0);
    lv_obj_set_style_text_font(label_date, &font_normal_32, 0);
    lv_obj_set_style_text_color(label_date, lv_color_hex(0xc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);

    label_weekday = lv_label_create(date_cont);
    lv_label_set_text(label_weekday, "---");
    lv_obj_set_style_text_letter_space(label_weekday, 3, 0);
    lv_obj_set_style_text_font(label_weekday, &font_bold_32, 0);
    lv_obj_set_style_text_color(label_weekday, lv_color_hex(0xc0c0c0), LV_PART_MAIN | LV_STATE_DEFAULT);

    // Battery icon on top-left
    extern const lv_image_dsc_t image_battery_icon;
    img_battery = lv_image_create(home_screen);
    lv_image_set_src(img_battery, &image_battery_icon);
    lv_obj_set_align(img_battery, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(img_battery, 8, 8);
    lv_obj_set_style_img_recolor_opa(img_battery, LV_OPA_COVER, 0);
    lv_obj_set_style_img_recolor(img_battery, lv_color_hex(0x909090), 0);

    // Battery percent label next to icon
    lbl_batt_pct = lv_label_create(home_screen);
    lv_obj_set_align(lbl_batt_pct, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(lbl_batt_pct, 8 + 53 + 8, 16); // icon width + padding
    lv_label_set_text(lbl_batt_pct, "--%");
    lv_obj_set_style_text_font(lbl_batt_pct, &font_normal_26, 0);

    // Charging lightning overlay on top of battery icon
    lbl_charge_icon = lv_label_create(img_battery);
#ifdef LV_SYMBOL_CHARGE
    lv_label_set_text(lbl_charge_icon, LV_SYMBOL_CHARGE);
#else
    lv_label_set_text(lbl_charge_icon, "⚡");
#endif
    lv_obj_center(lbl_charge_icon);
    // Use default LVGL font to ensure symbol glyphs are present
    lv_obj_set_style_text_font(lbl_charge_icon, LV_FONT_DEFAULT, 0);
    lv_obj_set_style_text_color(lbl_charge_icon, lv_color_white(), 0);
    lv_obj_add_flag(lbl_charge_icon, LV_OBJ_FLAG_HIDDEN);

    // BLE status icon on top-right
    extern const lv_image_dsc_t image_bluetooth_icon;
    img_ble = lv_image_create(home_screen);
    lv_image_set_src(img_ble, &image_bluetooth_icon);
    lv_obj_set_align(img_ble, LV_ALIGN_TOP_RIGHT);
    lv_obj_set_pos(img_ble, -8, 8);
    lv_obj_set_style_img_recolor_opa(img_ble, LV_OPA_COVER, 0);
    // Default to disconnected (grey)
    lv_obj_set_style_img_recolor(img_ble, lv_color_hex(0x606060), 0);

    lv_timer_create(update_time_task, 1000, NULL);
}

void watchface_set_power_state(bool vbus_in, bool charging, int battery_percent)
{
    (void)battery_percent; // future: render %, or change icon fill
    if (!img_battery) return;
    lv_color_t col = lv_color_hex(0x909090); // default: grey
    if (vbus_in) {
        col = lv_color_hex(0x00BFFF); // USB plugged: blue
    }
    if (charging) {
        col = lv_color_hex(0x00FF00); // Charging: green
    }
    lv_obj_set_style_img_recolor(img_battery, col, 0);
    // Update percent text
    if (lbl_batt_pct) {
        if (battery_percent >= 0 && battery_percent <= 100) {
            static char buf[8];
            lv_snprintf(buf, sizeof(buf), "%d%%", battery_percent);
            lv_label_set_text(lbl_batt_pct, buf);
        } else {
            lv_label_set_text(lbl_batt_pct, "--%");
        }
    }
    // Toggle lightning overlay: show if VBUS present or charging
    if (lbl_charge_icon) {
        if (vbus_in || charging) {
            lv_obj_clear_flag(lbl_charge_icon, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(lbl_charge_icon, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void watchface_set_ble_connected(bool connected)
{
    if (!img_ble) return;
    lv_color_t col = connected ? lv_color_hex(0x3B82F6) /* blue */ : lv_color_hex(0x606060) /* grey */;
    lv_obj_set_style_img_recolor(img_ble, col, 0);
}
