#include "lvgl.h"
#include "steps_screen.h"
#include "sensors.h"
#include "ui_fonts.h"
#include "settings.h"

#include "ui.h"
#include "watchface.h"


static lv_obj_t* step_screen = NULL;
static lv_obj_t* s_value_label = NULL;
static lv_obj_t* s_goal_label = NULL;
static lv_obj_t* s_activity_label = NULL;
static lv_obj_t* s_bar = NULL;
static lv_obj_t* s_ticks[4] = { 0 };

static lv_obj_t* s_icon_left = NULL;
//static lv_obj_t* s_icon_right = NULL;
static lv_timer_t* s_timer = NULL;
static uint32_t s_goal_steps = 8000;

LV_IMAGE_DECLARE(image_walk_48);

static void screen_events(lv_event_t* e);

static void steps_timer_cb(lv_timer_t* t)
{
    LV_UNUSED(t);
    bsp_display_lock(0);
    //if (active_screen_get() == step_screen) {
        //if (!s_value_label) return;
        // Refresh goal from settings if changed
        uint32_t new_goal = settings_get_step_goal();
        if (new_goal != s_goal_steps) {
            s_goal_steps = new_goal ? new_goal : 1;
            if (s_goal_label) {
                char gbuf2[32];
                lv_snprintf(gbuf2, sizeof(gbuf2), "Goal %u", (unsigned)s_goal_steps);
                lv_label_set_text(s_goal_label, gbuf2);
            }
        }
        uint32_t steps = sensors_get_step_count();
        char buf[32];
        lv_snprintf(buf, sizeof(buf), "%u", (unsigned)steps);
        lv_label_set_text(s_value_label, buf);

        // Update progress and percent
        uint32_t goal = s_goal_steps ? s_goal_steps : 1;
        uint32_t pct = (steps >= goal) ? 100 : (steps * 100u) / goal;
        if (s_bar) lv_bar_set_value(s_bar, (int32_t)pct, LV_ANIM_OFF);

        // Update activity type label
        if (s_activity_label) {
            sensors_activity_t act = sensors_get_activity();
            const char* text = "Idle";
            switch (act) {
            case SENSORS_ACTIVITY_WALK: text = "Walk"; break;
            case SENSORS_ACTIVITY_RUN:  text = "Run";  break;
            case SENSORS_ACTIVITY_OTHER:text = "Active"; break;
            case SENSORS_ACTIVITY_IDLE:
            default: text = "Idle"; break;
            }
            lv_label_set_text(s_activity_label, text);
        }
    //}

    bsp_display_unlock();
}

void steps_screen_create(lv_obj_t* parent)
{
    static lv_style_t cmain_style;

    lv_style_init(&cmain_style);
    lv_style_set_text_color(&cmain_style, lv_color_white());
    lv_style_set_bg_color(&cmain_style, lv_color_hex(0x000000));
    lv_style_set_bg_opa(&cmain_style, LV_OPA_100);

    step_screen = lv_obj_create(parent);
    lv_obj_remove_style_all(step_screen);
    lv_obj_set_size(step_screen, lv_pct(100), lv_pct(100));
    lv_obj_set_align(step_screen, LV_ALIGN_CENTER);
    lv_obj_add_style(step_screen, &cmain_style, 0);

    lv_obj_t* hdr_card = lv_obj_create(step_screen);
    lv_obj_remove_style_all(hdr_card);
    //lv_obj_set_size(hdr_card, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hdr_card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_obj_set_size(hdr_card, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_align(hdr_card, LV_ALIGN_TOP_MID);


    lv_obj_t* img = lv_image_create(hdr_card);
    //lv_obj_set_align(img, LV_ALIGN_TOP_MID);
    lv_image_set_src(img, &image_walk_48);

    // Title row
    lv_obj_t* title = lv_label_create(hdr_card);
    lv_label_set_text(title, "Steps");
    lv_obj_set_style_text_font(title, &font_bold_32, 0);
    lv_obj_set_style_text_letter_space(title, 1, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    //lv_obj_set_align(title, LV_ALIGN_TOP_MID);
    //lv_obj_set_y(title, 100);

    // Value label
    s_value_label = lv_label_create(step_screen);
    lv_obj_set_style_text_color(s_value_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_value_label, &font_numbers_80, 0);
    lv_label_set_text(s_value_label, "0");
    lv_obj_set_align(s_value_label, LV_ALIGN_CENTER);
    lv_obj_set_y(s_value_label, -20);

    // Goal text under value
    s_goal_label = lv_label_create(step_screen);
    // Initialize goal from settings
    s_goal_steps = settings_get_step_goal();
    lv_label_set_text_fmt(s_goal_label, "Goal %u", (unsigned)s_goal_steps);
    lv_obj_set_style_text_color(s_goal_label, lv_color_hex(0x909090), 0);
    lv_obj_align_to(s_goal_label, s_value_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_text_font(s_goal_label, &font_normal_32, 0);

    // Activity label under goal
    s_activity_label = lv_label_create(step_screen);
    lv_label_set_text(s_activity_label, "Idle");
    lv_obj_set_style_text_color(s_activity_label, lv_color_hex(0xA0A0A0), 0);
    lv_obj_set_style_text_font(s_activity_label, &font_normal_32, 0);
    lv_obj_align_to(s_activity_label, s_goal_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);

    // Horizontal progress bar near bottom
    s_bar = lv_bar_create(step_screen);
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
    s_icon_left = lv_label_create(step_screen);
    lv_label_set_text(s_icon_left, LV_SYMBOL_OK);
    lv_obj_set_style_text_color(s_icon_left, lv_color_hex(0x606060), 0);
    lv_obj_align_to(s_icon_left, s_bar, LV_ALIGN_OUT_LEFT_MID, -10, 0);

    /*s_icon_right = lv_label_create(step_screen);
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

    s_timer = lv_timer_create(steps_timer_cb, 5000, NULL);
    lv_timer_ready(s_timer);

    //lv_obj_add_event_cb(step_screen, screen_events, LV_EVENT_GESTURE, NULL);
}

static void screen_events(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
        if (dir == LV_DIR_LEFT) {
            lv_indev_wait_release(lv_indev_active());
            load_screen(step_screen, watchface_screen_get(), LV_SCR_LOAD_ANIM_MOVE_LEFT);
        }
    }
    else if (lv_event_get_code(e) == LV_EVENT_SCREEN_LOADED) {

    }
}

lv_obj_t* steps_screen_get(void)
{
    if (step_screen == NULL) {
        // Create as a standalone screen if not yet created
        steps_screen_create(NULL);
    }
    return step_screen;
}

void steps_screen_set_goal(uint32_t goal_steps)
{
    s_goal_steps = goal_steps ? goal_steps : 1;
    if (s_goal_label) {
        lv_label_set_text_fmt(s_goal_label, "Goal %u", (unsigned)s_goal_steps);
    }
}

