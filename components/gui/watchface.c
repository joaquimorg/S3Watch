#include "watchface.h"
#include "sensors.h"
#include "settings_screen.h"
#include "ui_private.h"
#include "rtc_lib.h"

LV_FONT_DECLARE(font_roboto_serif_bold_164)
LV_FONT_DECLARE(font_inter_regular_28)
LV_FONT_DECLARE(font_inter_bold_28);

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

static bool inited = false;

static lv_style_t main_style;

static uint32_t arc_colors[] = {

    0xB70074, /* Purple */
    0x792672, /* Violet */
    0x193E8D, /* Deep Blue */
    0x179DD6, /* Light Blue */
    0x009059, /* Dark Green */
    0x5FB136, /* Light Green */
    0xF9EE19, /* Yellow */
    0xF6D10E, /* Ocher */
    0xEF7916, /* Cream */

    0xEA4427, /* Orange */
    0xEB0F13, /* Red */
    0xD50059, /* Garnet */
};

static lv_anim_t rotate;

lv_obj_t * arc_cont;
lv_obj_t * main_arc;
lv_obj_t * overlay;


static void translate_x_anim(void * var, int32_t v)
{
    lv_obj_set_style_translate_x(var, v, 0);
}

static void translate_y_anim(void * var, int32_t v)
{
    lv_obj_set_style_translate_y(var, v, 0);
}

static void opa_anim(void * var, int32_t v)
{
    lv_obj_set_style_opa(var, v, 0);
}

static void rotate_image(void * var, int32_t v)
{
    lv_arc_set_rotation(var, v);
}

void lv_smartwatch_animate_x(lv_obj_t * obj, int32_t x, int32_t duration, int32_t delay)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_duration(&a, duration);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_values(&a, lv_obj_get_style_translate_x(obj, 0), x);
    lv_anim_set_exec_cb(&a, translate_x_anim);
    lv_anim_start(&a);
}

void lv_smartwatch_animate_x_from(lv_obj_t * obj, int32_t start, int32_t x, int32_t duration, int32_t delay)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_duration(&a, duration);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_values(&a, start, x);
    lv_anim_set_exec_cb(&a, translate_x_anim);
    lv_anim_start(&a);
}

void lv_smartwatch_animate_y(lv_obj_t * obj, int32_t y, int32_t duration, int32_t delay)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_duration(&a, duration);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_values(&a, lv_obj_get_style_translate_y(obj, 0), y);
    lv_anim_set_exec_cb(&a, translate_y_anim);
    lv_anim_start(&a);
}

void lv_smartwatch_anim_opa(lv_obj_t * obj, lv_opa_t opa, int32_t duration, int32_t delay)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_duration(&a, duration);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_values(&a, lv_obj_get_style_opa(obj, 0), opa);
    lv_anim_set_exec_cb(&a, opa_anim);
    lv_anim_start(&a);
}

static void shrink_to_left(void * var, int32_t v)
{
    lv_obj_t * parent = var;
    for(uint32_t i = 0; i < lv_obj_get_child_count(parent); i++) {
        lv_obj_t * obj = lv_obj_get_child(parent, i);
        int32_t orig = ARC_POS(i);
        int32_t target = 165; /* Left side is 180 - (half_arc) [180 - 15]*/
        int32_t final = orig + ((target - orig) * v) / 100;
        int32_t thick = 27 + ((5 - 27) * v) / 100;
        lv_color_hsv_t hsv = lv_color_to_hsv(lv_color_hex(arc_colors[i]));
        hsv.s = 100 - v;
        hsv.v = hsv.v + ((100 - hsv.v) * v) / 100;
        lv_arc_set_rotation(obj, final);
        lv_obj_set_style_arc_width(obj, thick, 0);
        lv_obj_set_style_arc_color(obj, lv_color_hsv_to_rgb(hsv.h, hsv.s, hsv.v), 0);
    }
}

static void shrink_to_down(void * var, int32_t v)
{
    lv_obj_t * parent = var;
    for(uint32_t i = 0; i < lv_obj_get_child_count(parent); i++) {
        lv_obj_t * obj = lv_obj_get_child(parent, i);
        int32_t orig = ARC_POS(i);
        int32_t target = 75;  /* Bottom side is 90 - (half_arc) [90 - 15]*/
        int32_t final = orig + ((target - orig) * v) / 100;
        int32_t thick = 27 + ((5 - 27) * v) / 100;
        lv_color_hsv_t hsv = lv_color_to_hsv(lv_color_hex(arc_colors[i]));
        hsv.s = 100 - v;
        hsv.v = hsv.v + ((100 - hsv.v) * v) / 100;
        lv_arc_set_rotation(obj, final);
        lv_obj_set_style_arc_width(obj, thick, 0);
        lv_obj_set_style_arc_color(obj, lv_color_hsv_to_rgb(hsv.h, hsv.s, hsv.v), 0);
    }
}

static void expand_from_left(void * var, int32_t v)
{
    lv_obj_t * parent = var;
    for(uint32_t i = 0; i < lv_obj_get_child_count(parent); i++) {
        lv_obj_t * obj = lv_obj_get_child(parent, i);
        int32_t orig = 165;
        int32_t target = ARC_POS(i);
        int32_t final = orig + ((target - orig) * v) / 100;
        int32_t thick = 5 + ((27 - 5) * v) / 100;
        lv_color_hsv_t hsv = lv_color_to_hsv(lv_color_hex(arc_colors[i]));
        hsv.s = v;
        hsv.v = 100 + ((hsv.v - 100) * v) / 100;
        lv_arc_set_rotation(obj, final);
        lv_obj_set_style_arc_width(obj, thick, 0);
        lv_obj_set_style_arc_color(obj, lv_color_hsv_to_rgb(hsv.h, hsv.s, hsv.v), 0);
    }
}

static void expand_from_down(void * var, int32_t v)
{
    lv_obj_t * parent = var;
    for(uint32_t i = 0; i < lv_obj_get_child_count(parent); i++) {
        lv_obj_t * obj = lv_obj_get_child(parent, i);
        int32_t orig = 75;
        int32_t target = ARC_POS(i);
        int32_t final = orig + ((target - orig) * v) / 100;
        int32_t thick = 5 + ((27 - 5) * v) / 100;
        lv_color_hsv_t hsv = lv_color_to_hsv(lv_color_hex(arc_colors[i]));
        hsv.s = v;
        hsv.v = 100 + ((hsv.v - 100) * v) / 100;
        lv_arc_set_rotation(obj, final);
        lv_obj_set_style_arc_width(obj, thick, 0);
        lv_obj_set_style_arc_color(obj, lv_color_hsv_to_rgb(hsv.h, hsv.s, hsv.v), 0);

    }

}

void lv_smartwatch_animate_arc(lv_obj_t * obj, lv_smartwatch_arc_animation_t animation, int32_t duration, int32_t delay)
{

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, 0, 100);
    lv_anim_set_duration(&a, duration);
    lv_anim_set_delay(&a, delay);
    switch(animation) {
        case ARC_SHRINK_DOWN:
            lv_anim_set_exec_cb(&a, shrink_to_down);
            break;
        case ARC_EXPAND_UP:
            lv_anim_set_exec_cb(&a, expand_from_down);
            break;
        case ARC_SHRINK_LEFT:
            lv_anim_set_exec_cb(&a, shrink_to_left);
            break;
        case ARC_EXPAND_RIGHT:
            lv_anim_set_exec_cb(&a, expand_from_left);
            break;
    }

    lv_anim_start(&a);
}

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

    //lv_obj_add_event_cb(home_screen, home_screen_events, LV_EVENT_ALL, NULL);

    /* Create background arcs */
    /*
    for(uint32_t i = 0; i < 12; i++) {
        lv_obj_t * bg_arc = lv_arc_create(home_screen);
        lv_obj_remove_style_all(bg_arc);
        lv_obj_set_size(bg_arc, lv_pct(100), lv_pct(100));
        lv_obj_set_align(bg_arc, LV_ALIGN_CENTER);
        lv_obj_set_y(bg_arc, 50);
        lv_arc_set_bg_start_angle(bg_arc, 0);
        lv_arc_set_bg_end_angle(bg_arc, 30);
        lv_arc_set_rotation(bg_arc, ARC_POS(i));
        lv_obj_remove_flag(bg_arc, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_set_style_arc_width(bg_arc, lv_pct(50), 0);
        lv_obj_set_style_arc_rounded(bg_arc, false, 0);
        lv_obj_set_style_arc_color(bg_arc, lv_color_hex(arc_colors[i]), 0);
        lv_obj_set_style_arc_opa(bg_arc, 50, 0);

    }*/


    label_hour = lv_label_create(home_screen);
    lv_obj_set_y(label_hour, -80);
    lv_obj_set_align(label_hour, LV_ALIGN_CENTER);
    lv_label_set_text(label_hour, "--");
    lv_obj_set_style_text_letter_space(label_hour, 1, 0);
    lv_obj_set_style_text_font(label_hour, &font_roboto_serif_bold_164, 0);
    lv_obj_set_style_text_color(label_hour, lv_color_hex(0x909090), LV_PART_MAIN | LV_STATE_DEFAULT);

    label_minute = lv_label_create(home_screen);
    lv_obj_set_y(label_minute, 90);
    lv_obj_set_align(label_minute, LV_ALIGN_CENTER);
    lv_label_set_text(label_minute, "--");
    lv_obj_set_style_text_letter_space(label_minute, 1, 0);
    lv_obj_set_style_text_font(label_minute, &font_roboto_serif_bold_164, 0);
    lv_obj_set_style_text_color(label_minute, lv_color_hex(0x909090), LV_PART_MAIN | LV_STATE_DEFAULT);

    label_second = lv_label_create(home_screen);
    lv_obj_set_align(label_second, LV_ALIGN_CENTER);
    lv_label_set_text(label_second, "--");
    lv_obj_set_style_text_letter_space(label_second, 1, 0);
    lv_obj_set_style_text_font(label_second, &font_inter_bold_28, 0);
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
    lv_obj_set_style_text_font(label_date, &font_inter_regular_28, 0);
    lv_obj_set_style_text_color(label_date, lv_color_hex(0x909090), LV_PART_MAIN | LV_STATE_DEFAULT);

    label_weekday = lv_label_create(date_cont);
    lv_label_set_text(label_weekday, "---");
    lv_obj_set_style_text_letter_space(label_weekday, 3, 0);
    lv_obj_set_style_text_font(label_weekday, &font_inter_bold_28, 0);
    lv_obj_set_style_text_color(label_weekday, lv_color_hex(0x909090), LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Container for the outer arcs */
    arc_cont = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(arc_cont);
    lv_obj_set_size(arc_cont, lv_pct(100), lv_pct(100));
    lv_obj_remove_flag(arc_cont, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);

    /* Create the outer arcs */
    for(uint32_t i = 0; i < 12; i++) {
        lv_obj_t * obj = lv_arc_create(arc_cont);
        lv_obj_remove_style_all(obj);
        lv_obj_set_size(obj, lv_pct(100), lv_pct(100));
        lv_obj_set_align(obj, LV_ALIGN_CENTER);
        //lv_obj_set_y(obj, 50);
        lv_arc_set_bg_start_angle(obj, 0);
        lv_arc_set_bg_end_angle(obj, 30);
        lv_arc_set_rotation(obj, ARC_POS(i));
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);

        lv_obj_set_style_arc_width(obj, 27, 0);
        lv_obj_set_style_arc_rounded(obj, false, 0);
        lv_obj_set_style_arc_color(obj, lv_color_hex(arc_colors[i]), 0);
        lv_obj_set_style_arc_opa(obj, 255, 0);

    }


    /* The rotating arc */
    /*main_arc = lv_arc_create(lv_screen_active());
    lv_obj_remove_style_all(main_arc);
    lv_obj_set_size(main_arc, lv_pct(100), lv_pct(100));
    lv_obj_set_align(main_arc, LV_ALIGN_CENTER);
    lv_obj_set_y(main_arc, 50);
    lv_arc_set_bg_start_angle(main_arc, 0);
    lv_arc_set_bg_end_angle(main_arc, 25);
    lv_arc_set_rotation(main_arc, 0);
    lv_obj_remove_flag(main_arc, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_set_style_arc_width(main_arc, 27, 0);
    lv_obj_set_style_arc_rounded(main_arc, false, 0);
    lv_obj_set_style_arc_color(main_arc, lv_color_white(), 0);
    lv_obj_set_style_arc_opa(main_arc, 255, 0);
    lv_obj_set_style_blend_mode(main_arc, LV_BLEND_MODE_DIFFERENCE, 0);
    lv_obj_set_style_opa(main_arc, 255, 0);*/

    /* Animate the rotating arc */
    /*lv_anim_init(&rotate);
    lv_anim_set_var(&rotate, main_arc);
    lv_anim_set_values(&rotate, 0, 360);
    lv_anim_set_duration(&rotate, 60000);
    lv_anim_set_exec_cb(&rotate, rotate_image);
    lv_anim_set_repeat_count(&rotate, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&rotate);*/

     /* Black overlay for screen transitions */
    /*overlay = lv_obj_create(home_screen);
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, lv_pct(100), lv_pct(100));
    lv_obj_remove_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(overlay, 255, 0);
    lv_obj_set_style_opa(overlay, 0, 0);

    lv_demo_smartwatch_control_create();
    lv_demo_smartwatch_health_create();*/

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
