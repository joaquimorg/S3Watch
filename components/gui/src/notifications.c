#include "notifications.h"
#include "ui_fonts.h"
#include <string.h>
#include <strings.h>

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#include "ui.h"
#include "watchface.h"

// Keep the last N notifications and allow swipe left/right
#define MAX_NOTIFICATIONS 5

LV_IMAGE_DECLARE(image_notification_48);
LV_IMAGE_DECLARE(image_sms_48);
LV_IMAGE_DECLARE(image_call_48);
LV_IMAGE_DECLARE(image_gmail_48);
LV_IMAGE_DECLARE(image_whatsapp_48);
LV_IMAGE_DECLARE(image_messenger_48);
LV_IMAGE_DECLARE(image_telegram_48);
LV_IMAGE_DECLARE(image_outlook_48);
LV_IMAGE_DECLARE(image_youtube_48);
LV_IMAGE_DECLARE(image_teams_48);
LV_IMAGE_DECLARE(image_instagram_48);
LV_IMAGE_DECLARE(image_tiktok_48);
LV_IMAGE_DECLARE(image_x_48);

typedef struct NotificationItem {
    char app[32];
    char title[64];
    char message[256];
    char ts_iso[40];
} NotificationItem;

// Data buffer
static NotificationItem notif_buf[MAX_NOTIFICATIONS];
static int notif_count = 0; // valid items in buffer

// Container and single reusable card (low memory)
static lv_obj_t *notification_screen;      // root container (fills panel)
static lv_obj_t *card;            // single card reused for all items
static lv_obj_t *avatar_img;      // optional image icon
static lv_obj_t *lbl_app;
static lv_obj_t *lbl_time;
static lv_obj_t *lbl_title;
static lv_obj_t *lbl_message;
static lv_obj_t *hdr_separator;
static lv_obj_t *pager_cont;      // bottom-center pager (dots)
static lv_obj_t *pager_dots[MAX_NOTIFICATIONS];
static int active_idx = 0;        // current shown index (0 = most recent)

lv_obj_t* notifications_screen_get(void);

static void set_label_text(lv_obj_t* lbl, const char* txt)
{
    if (!lbl) return;
    if (!txt) txt = "";
    lv_label_set_text(lbl, txt);
}

static void show_empty_state(void)
{
    if (avatar_img) {
        lv_image_set_src(avatar_img, &image_notification_48);
    }
    set_label_text(lbl_app, "Notifications");
    set_label_text(lbl_title, "You don't have\nnew notifications...");
    set_label_text(lbl_message, "");
    set_label_text(lbl_time, "");
}

// Format "YYYY-MM-DD HH:MM" from ISO timestamp
static void format_datetime_ymd_hhmm(const char* iso_ts, char* out, size_t out_sz)
{
    if (!iso_ts || !out || out_sz < 17) { if (out && out_sz) out[0] = '\0'; return; }
    const char* t = NULL;
    for (const char* p = iso_ts; *p; ++p) {
        if (*p == 'T' || *p == 't' || *p == ' ') { t = p; break; }
    }
    if (!t) { out[0] = '\0'; return; }
    if ((t - iso_ts) < 10) { out[0] = '\0'; return; }
    if (!(t[1] && t[2] && t[3] == ':' && t[4] && t[5])) { out[0] = '\0'; return; }
    memcpy(out, iso_ts, 10);
    out[10] = ' ';
    out[11] = t[1]; out[12] = t[2]; out[13] = ':'; out[14] = t[4]; out[15] = t[5]; out[16] = '\0';
}

typedef struct AppMeta {
    const char* id;
    const char* friendly;    
    const lv_image_dsc_t* icon;
} AppMeta;

static const AppMeta k_known_apps[] = {
    { "sms",                                "SMS",          &image_sms_48 },
    { "com.android.messaging",              "SMS",          &image_sms_48 },
    { "com.google.android.apps.messaging",  "SMS",          &image_sms_48 },
    { "com.google.android.apps.messagi",    "SMS",          &image_sms_48 },
    { "call",                               "Call",         &image_call_48 },
    { "com.google.android.dialer",          "Call",         &image_call_48 },
    { "com.google.android.gm",              "Gmail",        &image_gmail_48 },
    { "com.google.android.youtube",         "YouTube",      &image_youtube_48 },
    { "com.whatsapp",                       "WhatsApp",     &image_whatsapp_48 },
    { "com.facebook.katana",                "Facebook",     &image_messenger_48 },
    { "org.telegram.messenger",             "Telegram",     &image_telegram_48 },
    { "com.microsoft.office.outlook",       "Outlook",      &image_outlook_48 },
    { "com.microsoft.teams",                "Teams",        &image_teams_48 },
    { "com.instagram.android",              "Instagram",    &image_instagram_48 },
    { "com.zhiliaoapp.musically",           "TikTok",       &image_tiktok_48 },
    { "com.twitter.android",                "X (Twitter)",  &image_x_48 },
};

static const AppMeta* get_app_meta(const char* app_id)
{
    static AppMeta dyn; // for unknown apps
    if (app_id && *app_id) {
        for (size_t i = 0; i < sizeof(k_known_apps)/sizeof(k_known_apps[0]); ++i) {
            if (strcasecmp(app_id, k_known_apps[i].id) == 0) {
                return &k_known_apps[i];
            }
        }
        dyn.id = app_id;
        dyn.friendly = app_id;
        dyn.icon = &image_notification_48;
        return &dyn;
    }
    static const AppMeta unknown = { "", "Notifications", &image_notification_48 };
    return &unknown;
}

static void update_card_content(int idx)
{
    if (idx < 0 || idx >= notif_count) {
        show_empty_state();
        return;
    }
    char dt[17];
    format_datetime_ymd_hhmm(notif_buf[idx].ts_iso, dt, sizeof(dt));
    /*if (lbl_num[idx]) {
        char num[8];
        lv_snprintf(num, sizeof(num), "# %d", idx + 1); // Latest is 1
        lv_label_set_text(lbl_num[idx], num);
    }*/
    // Unified metadata for name, color, icon
    const char* app_id = notif_buf[idx].app;
    const AppMeta* meta = get_app_meta(app_id);
    set_label_text(lbl_app, meta ? meta->friendly : app_id);
    set_label_text(lbl_title, notif_buf[idx].title);
    set_label_text(lbl_message, notif_buf[idx].message);
    set_label_text(lbl_time, dt);

    // Update avatar (image if available; otherwise colored monogram)
    //if (avatar) {

        // Always show an icon per metadata
        const lv_image_dsc_t* icon = (meta && meta->icon) ? meta->icon : &image_notification_48;
        if (icon && avatar_img) {
            lv_image_set_src(avatar_img, icon);
        }
        // Use color from metadata on the label text (safer than bg on label)
        //uint32_t color = meta ? meta->color : 0x3B82F6;
        //lv_obj_set_style_text_color(lbl_app, lv_color_hex(color), 0);

        // Letter avatar is no longer used (always show icon)
    //}
}

static void create_tile(int idx) { (void)idx; }

static void build_single_card(lv_obj_t* parent)
{
    card = lv_obj_create(parent);
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, lv_pct(100), lv_pct(100));
    //lv_obj_set_style_pad_all(card, 10, 0);
    //lv_obj_set_style_pad_row(card, 10, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_BETWEEN);
    //lv_obj_add_flag(card, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t* hdr_card = lv_obj_create(card);
    lv_obj_remove_style_all(hdr_card);
    //lv_obj_set_size(hdr_card, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hdr_card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_obj_set_size(hdr_card, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_align(hdr_card, LV_ALIGN_TOP_MID);
    lv_obj_set_style_bg_color(hdr_card, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(hdr_card, LV_OPA_20, 0);

    /*avatar = lv_obj_create(hdr_card);
    lv_obj_remove_style_all(avatar);
    lv_obj_set_size(avatar, 62, 62);
    lv_obj_set_style_radius(avatar, 18, 0);
    lv_obj_set_style_bg_color(avatar, lv_color_hex(0x444444), 0);
    lv_obj_set_style_bg_opa(avatar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(avatar, 0, 0);
    lv_obj_set_style_pad_all(avatar, 0, 0);*/

    avatar_img = lv_image_create(hdr_card);
    lv_image_set_src(avatar_img, &image_notification_48);
    //lv_obj_set_size(avatar_img, 62, 62);    

    lbl_app = lv_label_create(hdr_card);
    lv_obj_set_style_text_color(lbl_app, lv_color_hex(0xF0F0F0), 0);
    lv_obj_set_style_text_font(lbl_app, &font_normal_26, 0);
    lv_obj_set_style_pad_left(lbl_app, 12, 0);
    lv_label_set_text(lbl_app, "Notifications");
    lv_label_set_long_mode(lbl_app, LV_LABEL_LONG_SCROLL_CIRCULAR);

    /*hdr_separator = lv_obj_create(card);
    lv_obj_remove_style_all(hdr_separator);
    lv_obj_set_height(hdr_separator, 1);
    lv_obj_set_width(hdr_separator, lv_pct(100));
    lv_obj_set_style_bg_color(hdr_separator, lv_color_hex(0x4E4E4E), 0);
    lv_obj_set_style_bg_opa(hdr_separator, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_top(hdr_separator, 0, 0);
    lv_obj_set_style_pad_bottom(hdr_separator, 0, 0);
    lv_obj_set_style_margin_top(hdr_separator, 0, 0);*/
    //lv_obj_set_style_margin_bottom(hdr_separator, 8, 0);
    //lv_obj_set_size(lbl_app, lv_pct(100), 52);

    lbl_title = lv_label_create(card);
    lv_label_set_text(lbl_title, "You don't have\nnew notifications...");
    lv_obj_set_width(lbl_title, lv_pct(100));
    lv_label_set_long_mode(lbl_title, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(lbl_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x90F090), 0);
    lv_obj_set_style_text_font(lbl_title, &font_bold_26, 0);

    lbl_message = lv_label_create(card);
    lv_label_set_text(lbl_message, "");
    lv_obj_set_width(lbl_message, lv_pct(100));
    lv_label_set_long_mode(lbl_message, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(lbl_message, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_message, lv_color_hex(0xF0F0F0), 0);
    lv_obj_set_style_text_font(lbl_message, &font_normal_26, 0);

    lbl_time = lv_label_create(card);
    lv_obj_set_style_pad_bottom(lbl_time, 25, 0);
    lv_obj_set_style_text_color(lbl_time, lv_color_hex(0x909090), 0);
    lv_obj_set_style_text_font(lbl_time, &font_normal_26, 0);
    lv_label_set_text(lbl_time, "");
}

static void update_pager(int active_idx)
{
    if (!pager_cont) return;
    // Hide when there are 0 or 1 notifications
    if (notif_count <= 1) {
        lv_obj_add_flag(pager_cont, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_clear_flag(pager_cont, LV_OBJ_FLAG_HIDDEN);

    // Ensure dots exist up to MAX_NOTIFICATIONS and reflect notif_count
    const int inactive_sz = 6;     // px
    const int active_sz   = 10;    // px (bigger for current)
    const uint32_t col_inactive = 0x9CA3AF; // lighter gray for better visibility
    const uint32_t col_active   = 0xF6F6C2; // accent color matches title

    for (int i = 0; i < MAX_NOTIFICATIONS; ++i) {
        if (i >= notif_count) {
            if (pager_dots[i]) lv_obj_add_flag(pager_dots[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        if (!pager_dots[i]) {
            pager_dots[i] = lv_obj_create(pager_cont);
            lv_obj_remove_style_all(pager_dots[i]);
            lv_obj_set_style_bg_opa(pager_dots[i], LV_OPA_COVER, 0);
            lv_obj_set_style_radius(pager_dots[i], LV_RADIUS_CIRCLE, 0);
        }
        lv_obj_clear_flag(pager_dots[i], LV_OBJ_FLAG_HIDDEN);

        bool is_active = (i == active_idx);
        int sz = is_active ? active_sz : inactive_sz;
        lv_obj_set_size(pager_dots[i], sz, sz);
        lv_obj_set_style_bg_color(pager_dots[i], lv_color_hex(is_active ? col_active : col_inactive), 0);
    }
}

// Animation helpers (file scope)
static void notif_anim_set_y(void *var, int32_t v)
{
    lv_obj_set_style_translate_y((lv_obj_t*)var, v, 0);
}
static void notif_anim_set_opa(void *var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t*)var, (lv_opa_t)v, 0);
}
static void notif_anim_set_x(void *var, int32_t v)
{
    lv_obj_set_style_translate_x((lv_obj_t*)var, v, 0);
}

// Simple slide transition state
static bool notif_is_animating = false;
static int slide_target_idx = 0;
static int slide_dir = 0; // +1 = to left (next), -1 = to right (prev)

static void slide_phase2_ready_cb(lv_anim_t* a)
{
    (void)a;
    notif_is_animating = false;
}

static void slide_phase1_ready_cb(lv_anim_t* a)
{
    (void)a;
    // Swap content to target item
    active_idx = slide_target_idx;
    update_card_content(active_idx);
    update_pager(active_idx);

    // Prepare for slide-in from opposite side
    int32_t start_x = (slide_dir > 0) ? 30 : -30;
    if (card) {
        lv_obj_set_style_translate_x(card, start_x, 0);
        lv_obj_set_style_opa(card, 0, 0);

        // Slide-in
        lv_anim_t a_in; lv_anim_init(&a_in);
        lv_anim_set_var(&a_in, card);
        lv_anim_set_values(&a_in, start_x, 0);
        lv_anim_set_time(&a_in, 140);
        lv_anim_set_exec_cb(&a_in, notif_anim_set_x);
        lv_anim_set_ready_cb(&a_in, slide_phase2_ready_cb);
        lv_anim_start(&a_in);

        // Fade-in
        lv_anim_t a_in_f; lv_anim_init(&a_in_f);
        lv_anim_set_var(&a_in_f, card);
        lv_anim_set_values(&a_in_f, 0, 255);
        lv_anim_set_time(&a_in_f, 140);
        lv_anim_set_exec_cb(&a_in_f, notif_anim_set_opa);
        lv_anim_start(&a_in_f);
    } else {
        notif_is_animating = false;
    }
}

static void start_slide_to(int new_idx, int dir)
{
    if (!card || new_idx == active_idx) {
        // Fallback: no animation
        active_idx = new_idx;
        update_card_content(active_idx);
        update_pager(active_idx);
        notif_is_animating = false;
        return;
    }
    notif_is_animating = true;
    slide_target_idx = new_idx;
    slide_dir = dir;

    // Ensure in known state
    lv_obj_set_style_translate_x(card, 0, 0);
    lv_obj_set_style_opa(card, 255, 0);

    // Phase 1: slide-out slightly and fade-out
    int32_t end_x = (dir > 0) ? -30 : 30;
    lv_anim_t a_out; lv_anim_init(&a_out);
    lv_anim_set_var(&a_out, card);
    lv_anim_set_values(&a_out, 0, end_x);
    lv_anim_set_time(&a_out, 120);
    lv_anim_set_exec_cb(&a_out, notif_anim_set_x);
    lv_anim_set_ready_cb(&a_out, slide_phase1_ready_cb);
    lv_anim_start(&a_out);

    lv_anim_t a_out_f; lv_anim_init(&a_out_f);
    lv_anim_set_var(&a_out_f, card);
    lv_anim_set_values(&a_out_f, 255, 0);
    lv_anim_set_time(&a_out_f, 120);
    lv_anim_set_exec_cb(&a_out_f, notif_anim_set_opa);
    lv_anim_start(&a_out_f);
}

static void delete_notification_at(int idx)
{
    if (idx < 0 || idx >= notif_count) {
        return;
    }
    for (int i = idx; i < notif_count - 1; ++i) {
        notif_buf[i] = notif_buf[i + 1];
    }
    notif_count--;
    if (notif_count <= 0) {
        notif_count = 0;
        active_idx = 0;
        notif_is_animating = false;
        show_empty_state();
        update_pager(active_idx);
        return;
    }
    if (active_idx > idx) {
        active_idx--;
    }
    if (active_idx >= notif_count) {
        active_idx = notif_count - 1;
    }
    notif_is_animating = false;
    update_card_content(active_idx);
    update_pager(active_idx);
}

static void gesture_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    //ESP_LOGI("NOTIF", "Notif event: %d", code);
    static lv_point_t press_start = {0,0};
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
        //ESP_LOGI("NOTIF", "Notif event dir : %d", dir);
        if (!notif_is_animating) {
            if (dir == LV_DIR_LEFT) {
                if (active_idx + 1 < notif_count) {
                    lv_indev_wait_release(lv_indev_active());
                    start_slide_to(active_idx + 1, +1);
                }
            } else if (dir == LV_DIR_RIGHT) {
                if (active_idx > 0) {
                    lv_indev_wait_release(lv_indev_active());
                    start_slide_to(active_idx - 1, -1);
                }
            }
        }
        /*if (dir == LV_DIR_TOP) {
            lv_indev_wait_release(lv_indev_active());
            load_screen(notification_screen, watchface_screen_get(), LV_SCR_LOAD_ANIM_MOVE_TOP);
        }
        return;*/
    }
    if (code == LV_EVENT_LONG_PRESSED) {
        if (!notif_is_animating && notif_count > 0) {
            lv_indev_wait_release(lv_indev_active());
            delete_notification_at(active_idx);
        }
        return;
    }
    if (code == LV_EVENT_PRESSED) {
        lv_indev_get_point(lv_indev_active(), &press_start);
        return;
    }
    if (code == LV_EVENT_RELEASED) {
        lv_point_t now; lv_indev_get_point(lv_indev_active(), &now);
        int dx = now.x - press_start.x;
        if (!notif_is_animating) {
            if (dx <= -30) { // swipe left
                if (active_idx + 1 < notif_count) {
                    start_slide_to(active_idx + 1, +1);
                }
            } else if (dx >= 30) { // swipe right
                if (active_idx > 0) {
                    start_slide_to(active_idx - 1, -1);
                }
            }
        }
        return;
    }
}

void notifications_screen_create(lv_obj_t* parent)
{

    static lv_style_t cmain_style;

    lv_style_init(&cmain_style);
    lv_style_set_text_color(&cmain_style, lv_color_white());
    lv_style_set_bg_color(&cmain_style, lv_color_hex(0x000000));
    lv_style_set_bg_opa(&cmain_style, LV_OPA_100);

    // Root container (no scroll)
    notification_screen = lv_obj_create(parent);
    lv_obj_remove_style_all(notification_screen);
    lv_obj_set_size(notification_screen, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(notification_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(notification_screen, &cmain_style, 0);

    // Build one reusable card and enable swipe/press events on container
    // Listen to all events here; the handler filters by code.    
    lv_obj_add_flag(notification_screen, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_CLICKABLE);
    build_single_card(notification_screen);

    // Ensure gestures on the card bubble up to the container (where handler is attached)
    if (card) {
        lv_obj_add_flag(card, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_CLICKABLE);
    }

    // Pager indicator at bottom-center (dots with active highlight)
    pager_cont = lv_obj_create(notification_screen);
    lv_obj_remove_style_all(pager_cont);
    lv_obj_set_align(pager_cont, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(pager_cont, -8);
    lv_obj_set_size(pager_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(pager_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(pager_cont, 0, 0);
    lv_obj_set_style_pad_row(pager_cont, 0, 0);
    lv_obj_set_style_pad_column(pager_cont, 8, 0); // gap between dots
    for (int i = 0; i < MAX_NOTIFICATIONS; ++i) pager_dots[i] = NULL;
    lv_obj_add_flag(pager_cont, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_event_cb(notification_screen, gesture_event_cb, LV_EVENT_ALL, NULL);
}

lv_obj_t* notifications_screen_get(void)
{
    if (notification_screen == NULL) {
        // Create as a standalone screen if not yet created
        notifications_screen_create(NULL);
    }
    return notification_screen;
}


void notifications_show(const char* app,
                        const char* title,
                        const char* message,
                        const char* timestamp_iso8601)
{
    if (!notification_screen) return;
    if (!title && !message) return; // ignore empty

    // Shift older items down
    if (notif_count < MAX_NOTIFICATIONS) notif_count++;
    for (int i = notif_count - 1; i > 0; --i) {
        notif_buf[i] = notif_buf[i - 1];
    }

    // Copy new item into slot 0
    #define CPY(dst, src) do { \
        const char* s_ = (src) ? (src) : ""; \
        strncpy((dst), s_, sizeof(dst) - 1); \
        (dst)[sizeof(dst) - 1] = '\0'; \
    } while (0)

    CPY(notif_buf[0].app, app);
    CPY(notif_buf[0].title, title);
    CPY(notif_buf[0].message, message);
    CPY(notif_buf[0].ts_iso, timestamp_iso8601);

    // Jump to latest
    active_idx = 0;
    update_card_content(active_idx);
    update_pager(active_idx);

    // Subtle entrance animation for the newest card
    /*if (card) {
        // Start with slight offset + fade
        lv_obj_set_style_translate_y(card, 16, 0);
        lv_obj_set_style_opa(card, 0, 0);

        lv_anim_t a1; lv_anim_init(&a1);
        lv_anim_set_var(&a1, card);
        lv_anim_set_values(&a1, 16, 0);
        lv_anim_set_time(&a1, 180);
        lv_anim_set_exec_cb(&a1, notif_anim_set_y);
        lv_anim_start(&a1);

        lv_anim_t a2; lv_anim_init(&a2);
        lv_anim_set_var(&a2, card);
        lv_anim_set_values(&a2, 0, 255);
        lv_anim_set_time(&a2, 220);
        lv_anim_set_exec_cb(&a2, notif_anim_set_opa);
        lv_anim_start(&a2);
    }*/
}
