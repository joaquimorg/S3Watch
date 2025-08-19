#include "lvgl.h"
#include "steps_screen.h"
#include "sensors.h"

LV_FONT_DECLARE(font_numbers_80)

static lv_obj_t* s_container = NULL;
static lv_obj_t* s_value_label = NULL;
static lv_obj_t* s_goal_label = NULL;
static lv_obj_t* s_bar = NULL;
static lv_obj_t* s_ticks[4] = {0};

static lv_obj_t* s_icon_left = NULL;
static lv_obj_t* s_icon_right = NULL;
static lv_timer_t* s_timer = NULL;
static uint32_t s_goal_steps = 8000;

static void steps_timer_cb(lv_timer_t* t)
{
    LV_UNUSED(t);
    if (!s_value_label) return;
    uint32_t steps = sensors_get_step_count();
    char buf[32];
    lv_snprintf(buf, sizeof(buf), "%u", (unsigned)steps);
    lv_label_set_text(s_value_label, buf);

    // Update progress and percent
    uint32_t goal = s_goal_steps ? s_goal_steps : 1;
    uint32_t pct = (steps >= goal) ? 100 : (steps * 100u) / goal;
    if (s_bar) lv_bar_set_value(s_bar, (int32_t)pct, LV_ANIM_OFF);
}

void steps_screen_create(lv_obj_t* parent)
{
    s_container = lv_obj_create(parent);
    lv_obj_remove_style_all(s_container);
    lv_obj_set_size(s_container, lv_pct(100), lv_pct(100));
    lv_obj_set_align(s_container, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(s_container, LV_OPA_0, 0);

    // Title row
    lv_obj_t* title = lv_label_create(s_container);
    
    lv_label_set_text(title, "Steps");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_letter_space(title, 1, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, LV_FONT_DEFAULT, 0);
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(title, 10);

    // Value label
    s_value_label = lv_label_create(s_container);
    lv_obj_set_style_text_color(s_value_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_value_label, &font_numbers_80, 0);
    lv_label_set_text(s_value_label, "0");
    lv_obj_set_align(s_value_label, LV_ALIGN_CENTER);
    lv_obj_set_y(s_value_label, -38);

    // Goal text under value
    s_goal_label = lv_label_create(s_container);
    char gbuf[32];
    lv_snprintf(gbuf, sizeof(gbuf), "Goal %u", (unsigned)s_goal_steps);
    lv_label_set_text(s_goal_label, gbuf);
    lv_obj_set_style_text_color(s_goal_label, lv_color_hex(0x909090), 0);
    lv_obj_align_to(s_goal_label, s_value_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);

    // Horizontal progress bar near bottom
    s_bar = lv_bar_create(s_container);
    lv_obj_set_size(s_bar, 270, 14);
    lv_obj_set_align(s_bar, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(s_bar, -50);
    lv_bar_set_range(s_bar, 0, 100);
    lv_bar_set_value(s_bar, 0, LV_ANIM_OFF);
    // Style bar (rounded pill)
    lv_obj_set_style_radius(s_bar, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_radius(s_bar, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x303030), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_bar, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x3B82F6), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_bar, LV_OPA_100, LV_PART_INDICATOR);

    // Icons on bar ends
    s_icon_left = lv_label_create(s_container);
    lv_label_set_text(s_icon_left, LV_SYMBOL_OK);
    lv_obj_set_style_text_color(s_icon_left, lv_color_hex(0x606060), 0);
    lv_obj_align_to(s_icon_left, s_bar, LV_ALIGN_OUT_LEFT_MID, -10, 0);

    /*s_icon_right = lv_label_create(s_container);
    lv_label_set_text(s_icon_right, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_color(s_icon_right, lv_color_hex(0x909090), 0);
    lv_obj_align_to(s_icon_right, s_bar, LV_ALIGN_OUT_RIGHT_MID, 40, 0);*/

    // Tick marks at 20/40/60/80%
    for (int i = 0; i < 4; ++i) {
        s_ticks[i] = lv_obj_create(s_bar);
        lv_obj_remove_style_all(s_ticks[i]);
        lv_obj_set_size(s_ticks[i], 2, 14);
        lv_obj_set_style_bg_color(s_ticks[i], lv_color_hex(0x808080), 0);
        lv_obj_set_style_bg_opa(s_ticks[i], LV_OPA_60, 0);
        int x = (270 * (i + 1)) / 5; // 60,120,180,240px from left
        lv_obj_align_to(s_ticks[i], s_bar, LV_ALIGN_LEFT_MID, x, 0);
    }

    // Periodic update every second
    if (!s_timer) {
        s_timer = lv_timer_create(steps_timer_cb, 1000, NULL);
        lv_timer_ready(s_timer);
    }
}

void steps_screen_set_goal(uint32_t goal_steps)
{
    s_goal_steps = goal_steps ? goal_steps : 1;
    if (s_goal_label) {
        char gbuf[32];
        lv_snprintf(gbuf, sizeof(gbuf), "Goal %u", (unsigned)s_goal_steps);
        lv_label_set_text(s_goal_label, gbuf);
    }
}
