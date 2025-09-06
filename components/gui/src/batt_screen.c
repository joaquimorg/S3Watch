#include "batt_screen.h"
#include "settings.h"
#include "ui_fonts.h"
#include "ui.h"
#include "settings_screen.h"
#include "bsp/esp32_s3_touch_amoled_2_06.h"
#include "esp_log.h"

// Access UI primitives via ui.h accessors
static const char* TAG = "BatteryScreen";

LV_IMAGE_DECLARE(image_battery_48);

static lv_obj_t* batt_screen;
static lv_obj_t* batt_percent_label;
static lv_obj_t* batt_bar;
static lv_obj_t* chip_source;
static lv_obj_t* chip_charge;
static lv_obj_t* row_vbat_val;
static lv_obj_t* row_vbus_val;
static lv_obj_t* row_vsys_val;
static lv_obj_t* row_temp_val;
static lv_timer_t* batt_timer;

static void batt_screen_events(lv_event_t* e);
static void batt_update_cb(lv_timer_t* t);
static void batt_update_values(void);
static lv_obj_t* make_chip(lv_obj_t* parent, const char* txt);
static lv_obj_t* make_row(lv_obj_t* parent, const char* label_txt, lv_obj_t** out_val);
static void on_delete(lv_event_t* e);

void lv_smartwatch_batt_create(lv_obj_t* screen)
{

    static lv_style_t cmain_style;


    lv_style_init(&cmain_style);
    lv_style_set_text_color(&cmain_style, lv_color_white());
    lv_style_set_bg_color(&cmain_style, lv_color_hex(0x000000));
    lv_style_set_bg_opa(&cmain_style, LV_OPA_100);


    batt_screen = lv_obj_create(screen);
    lv_obj_remove_style_all(batt_screen);
    lv_obj_add_style(batt_screen, &cmain_style, 0);
    lv_obj_set_size(batt_screen, lv_pct(100), lv_pct(100));
    // Let gestures bubble to tileview so horizontal swipes work
    //lv_obj_add_flag(batt_screen, LV_OBJ_FLAG_GESTURE_BUBBLE);
    //lv_obj_remove_flag(batt_screen, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_clear_flag(batt_screen, LV_OBJ_FLAG_SCROLLABLE);

    // Layout: vertical column
    lv_obj_set_flex_flow(batt_screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(batt_screen, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(batt_screen, 16, 0);

    lv_obj_t* hdr_card = lv_obj_create(batt_screen);
    lv_obj_remove_style_all(hdr_card);
    //lv_obj_set_size(hdr_card, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hdr_card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_obj_set_size(hdr_card, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_align(hdr_card, LV_ALIGN_TOP_MID);


    lv_obj_t* img = lv_image_create(hdr_card);
    //lv_obj_set_align(img, LV_ALIGN_TOP_MID);
    lv_image_set_src(img, &image_battery_48);

    lv_obj_t* batt_title_label = lv_label_create(hdr_card);
    lv_obj_set_style_text_font(batt_title_label, &font_bold_32, 0);
    lv_label_set_text(batt_title_label, "Battery");

    // Big percentage label
    batt_percent_label = lv_label_create(batt_screen);
    lv_obj_set_style_text_font(batt_percent_label, &font_bold_42, 0);
    lv_label_set_text(batt_percent_label, "--%");

    // Battery bar (0..100%)
    batt_bar = lv_bar_create(batt_screen);
    lv_obj_set_width(batt_bar, lv_pct(90));
    lv_obj_set_height(batt_bar, 18);
    lv_bar_set_range(batt_bar, 0, 100);
    lv_bar_set_value(batt_bar, 0, LV_ANIM_OFF);

    // Status chips (source + charging)
    lv_obj_t* status = lv_obj_create(batt_screen);
    lv_obj_remove_style_all(status);
    lv_obj_set_width(status, lv_pct(90));
    lv_obj_set_style_pad_all(status, 0, 0);
    lv_obj_set_flex_flow(status, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(status, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_height(status, LV_SIZE_CONTENT);

    
    (void)make_row(status, "Source", &chip_source);
    (void)make_row(status, "Charging", &chip_charge);
    (void)make_row(status, "VBAT", &row_vbat_val);
    (void)make_row(status, "VBUS", &row_vbus_val);
    (void)make_row(status, "VSYS", &row_vsys_val);
    (void)make_row(status, "Temp", &row_temp_val);

    // Periodic refresh
    batt_timer = lv_timer_create(batt_update_cb, 5000, NULL);
    //lv_timer_ready(batt_timer);
    batt_update_values();

    lv_obj_add_event_cb(batt_screen, batt_screen_events, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(batt_screen, on_delete, LV_EVENT_DELETE, NULL);

}

static void on_delete(lv_event_t* e)
{
    (void)e;
    ESP_LOGI(TAG, "Battery screen deleted");
    if (batt_timer) { lv_timer_del(batt_timer); batt_timer = NULL; }
    batt_screen = NULL;
}

lv_obj_t* batt_screen_get(void)
{
    if (batt_screen == NULL) {
        // Create as a standalone screen if not yet created
        lv_smartwatch_batt_create(NULL);
    }
    return batt_screen;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void batt_screen_events(lv_event_t* e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    // Log Events
    //ESP_LOGI(TAG, "Battery screen event: %d", event_code);

    if (event_code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());

        if(dir == LV_DIR_RIGHT) {
            lv_indev_wait_release(lv_indev_active());
            // Return to controls tile and remove dynamic tile
            ui_dynamic_tile_close();
            if (batt_timer) { lv_timer_del(batt_timer); batt_timer = NULL; }
            batt_screen = NULL;
            //lv_obj_del_async(batt_screen);
        } 
    }
}

static void batt_update_cb(lv_timer_t* t)
{
    (void)t;
    if (active_screen_get() == batt_screen) {
        bsp_display_lock(0);
        batt_update_values();
        bsp_display_unlock();
    }
}

static void batt_update_values(void)
{
    int pct = bsp_power_get_battery_percent();
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    lv_bar_set_value(batt_bar, pct, LV_ANIM_ON);
    char txt[32];
    snprintf(txt, sizeof(txt), "%d%%", pct);
    lv_label_set_text(batt_percent_label, txt);

    int vbat = bsp_power_get_batt_voltage_mv();
    int vbus = bsp_power_get_vbus_voltage_mv();
    int vsys = bsp_power_get_system_voltage_mv();
    float temp = bsp_power_get_temperature_c();
    bool chg = bsp_power_is_charging();
    bool vbus_in = bsp_power_is_vbus_in();

    // Chips: Source + Charging
    char buf[48];
    //snprintf(buf, sizeof(buf), "Source: %s", vbus_in ? "USB" : "Battery");
    lv_label_set_text(chip_source, vbus_in ? "USB" : "Battery");
    //snprintf(buf, sizeof(buf), "Charging: %s", chg ? "Yes" : "No");
    lv_label_set_text(chip_charge, chg ? "Yes" : "No");
    lv_obj_set_style_text_color(chip_charge, chg ? lv_color_hex(0x2ECC71) : lv_color_hex(0xFFFFFF), 0);
    //lv_obj_set_style_bg_opa(chip_charge, chg ? LV_OPA_30 : LV_OPA_20, 0);

    lv_obj_set_style_text_color(batt_percent_label, chg ? lv_color_hex(0x2ECC71) : lv_color_hex(0xFFFFFF), 0);
    //lv_obj_set_style_bg_opa(chip_charge, chg ? LV_OPA_30 : LV_OPA_20, 0);

    // Values: format as volts with 2 decimals when valid
    if (vbat > 0) {
        snprintf(buf, sizeof(buf), "%.2f V", vbat / 1000.0f);
    } else {
        snprintf(buf, sizeof(buf), "n/a");
    }
    lv_label_set_text(row_vbat_val, buf);

    if (vbus > 0) {
        snprintf(buf, sizeof(buf), "%.2f V", vbus / 1000.0f);
    } else {
        snprintf(buf, sizeof(buf), "n/a");
    }
    lv_label_set_text(row_vbus_val, buf);

    if (vsys > 0) {
        snprintf(buf, sizeof(buf), "%.2f V", vsys / 1000.0f);
    } else {
        snprintf(buf, sizeof(buf), "n/a");
    }
    lv_label_set_text(row_vsys_val, buf);

    snprintf(buf, sizeof(buf), "%.1f %s", (double)temp, "°C");
    lv_label_set_text(row_temp_val, buf);
}

static lv_obj_t* make_chip(lv_obj_t* parent, const char* txt)
{
    lv_obj_t* chip = lv_label_create(parent);
    lv_label_set_text(chip, txt);
    lv_obj_set_style_text_font(chip, &font_normal_26, 0);
    lv_obj_set_style_bg_opa(chip, LV_OPA_20, 0);
    lv_obj_set_style_bg_color(chip, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(chip, 10, 0);
    lv_obj_set_style_pad_hor(chip, 12, 0);
    lv_obj_set_style_pad_ver(chip, 6, 0);
    lv_obj_set_style_margin_right(chip, 8, 0);
    lv_obj_set_style_margin_bottom(chip, 8, 0);
    return chip;
}

static lv_obj_t* make_row(lv_obj_t* parent, const char* label_txt, lv_obj_t** out_val)
{
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(row, 4, 0);

    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_t* l = lv_label_create(row);
    lv_label_set_text(l, label_txt);
    lv_obj_set_style_text_color(l, lv_color_hex(0xB0B0B0), 0);
    lv_obj_set_style_text_font(l, &font_normal_26, 0);
    lv_obj_t* v = lv_label_create(row);
    lv_obj_set_style_text_font(v, &font_bold_28, 0);
    lv_label_set_text(v, "--");
    if (out_val) *out_val = v;
    return row;
}
