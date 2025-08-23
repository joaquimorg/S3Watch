#include "setting_step_goal_screen.h"
#include "ui.h"
#include "ui_fonts.h"
#include "settings.h"

static lv_obj_t* sstepgoal_screen;
static lv_obj_t* sstepgoal_value;

static void screen_events(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_GESTURE) {
        if (lv_indev_get_gesture_dir(lv_indev_active()) == LV_DIR_RIGHT) {
            extern lv_obj_t* settings_menu_screen_get(void);
            load_screen(settings_menu_screen_get(), LV_SCR_LOAD_ANIM_MOVE_RIGHT);
        }
    }
}

static void upd(void)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", (unsigned)settings_get_step_goal());
    lv_label_set_text(sstepgoal_value, buf);
}

static void minus(lv_event_t* e){ (void)e; uint32_t g=settings_get_step_goal(); g = (g>1000)? g-1000:1000; settings_set_step_goal(g); upd(); }
static void plus(lv_event_t* e){ (void)e; uint32_t g=settings_get_step_goal(); g = (g<100000)? g+1000:100000; settings_set_step_goal(g); upd(); }

void setting_step_goal_screen_create(lv_obj_t* parent)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_color(&style, lv_color_white());
    lv_style_set_bg_color(&style, lv_color_black());
    lv_style_set_bg_opa(&style, LV_OPA_COVER);

    sstepgoal_screen = lv_obj_create(parent);
    lv_obj_remove_style_all(sstepgoal_screen);
    lv_obj_add_style(sstepgoal_screen, &style, 0);
    lv_obj_set_size(sstepgoal_screen, lv_pct(100), lv_pct(100));
    lv_obj_add_event_cb(sstepgoal_screen, screen_events, LV_EVENT_ALL, NULL);

    // Header
    lv_obj_t* hdr = lv_obj_create(sstepgoal_screen);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_size(hdr, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_t* title = lv_label_create(hdr);
    lv_obj_set_style_text_font(title, &font_bold_32, 0);
    lv_label_set_text(title, "Step Goal");

    // Value + buttons
    lv_obj_t* box = lv_obj_create(sstepgoal_screen);
    lv_obj_remove_style_all(box);
    lv_obj_set_size(box, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_center(box);
    //lv_obj_set_style_pad_top(box, 80, 0);
    //lv_obj_set_style_pad_bottom(box, 10, 0);
    lv_obj_set_style_pad_left(box, 12, 0);
    lv_obj_set_style_pad_right(box, 12, 0);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(box, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* btn_m = lv_btn_create(box);
    lv_obj_set_size(btn_m, 80, 80);
    lv_obj_add_event_cb(btn_m, minus, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lm = lv_label_create(btn_m);
    lv_obj_set_style_text_font(lm, &font_bold_42, 0);
    lv_label_set_text(lm, "-");

    sstepgoal_value = lv_label_create(box);
    lv_obj_set_style_text_font(sstepgoal_value, &font_numbers_80, 0);
    lv_label_set_text(sstepgoal_value, "--");

    lv_obj_t* btn_p = lv_btn_create(box);
    lv_obj_set_size(btn_p, 80, 80);
    lv_obj_add_event_cb(btn_p, plus, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lp = lv_label_create(btn_p);
    lv_obj_set_style_text_font(lp, &font_bold_42, 0);
    lv_label_set_text(lp, "+");

    upd();
}

lv_obj_t* setting_step_goal_screen_get(void)
{
    if (!sstepgoal_screen) setting_step_goal_screen_create(NULL);
    return sstepgoal_screen;
}

