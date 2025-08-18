#include "notifications.h"
#include "ui_private.h"

static lv_obj_t * health_screen;
static lv_obj_t * heart_bg_2;
static lv_obj_t * image_button;

void lv_smartwatch_notifications_create(lv_obj_t * screen)
{

    static lv_style_t main_style;

    lv_style_init(&main_style);
    lv_style_set_text_color(&main_style, lv_color_white());
    lv_style_set_bg_color(&main_style, lv_color_hex(0x000000));
    lv_style_set_bg_opa(&main_style, LV_OPA_100);



    lv_obj_t* label4 = lv_label_create(screen);
    lv_label_set_text(label4, "Messages");
    lv_obj_center(label4);


}