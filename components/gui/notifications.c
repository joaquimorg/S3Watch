#include "notifications.h"
#include "ui_fonts.h"
#include <string.h>
#include <strings.h>

// Keep the last N notifications and allow swipe left/right
#define MAX_NOTIFICATIONS 5

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
static lv_obj_t *notif_cont;      // root container (fills panel)
static lv_obj_t *card;            // single card reused for all items
static lv_obj_t *hdr_row;         // header row container
static lv_obj_t *avatar;          // avatar circle
static lv_obj_t *avatar_letter;   // monogram letter
static lv_obj_t *avatar_img;      // optional image icon
static lv_obj_t *lbl_app;
static lv_obj_t *lbl_time;
static lv_obj_t *lbl_title;
static lv_obj_t *lbl_message;
static lv_obj_t *lbl_pager;       // bottom-center pager "n/m"
static int active_idx = 0;        // current shown index (0 = most recent)

static void set_label_text(lv_obj_t* lbl, const char* txt)
{
    if (!lbl) return;
    if (!txt) txt = "";
    lv_label_set_text(lbl, txt);
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

/* Optional app icons (compiled if present). Symbols declared weak so missing files don't break the build. */
extern const lv_image_dsc_t image_notification_48 __attribute__((weak));
extern const lv_image_dsc_t image_sms_48 __attribute__((weak));
extern const lv_image_dsc_t image_call_48 __attribute__((weak));
extern const lv_image_dsc_t image_gmail_48 __attribute__((weak));
extern const lv_image_dsc_t image_whatsapp_48 __attribute__((weak));
extern const lv_image_dsc_t image_messenger_48 __attribute__((weak));
extern const lv_image_dsc_t image_telegram_48 __attribute__((weak));
extern const lv_image_dsc_t image_outlook_48 __attribute__((weak));
extern const lv_image_dsc_t image_youtube_48 __attribute__((weak));

static const lv_image_dsc_t* pick_app_icon(const char* app_id)
{
    if (!app_id) return &image_notification_48; /* may be NULL if not linked */
    if (strcasecmp(app_id, "sms") == 0 || strcmp(app_id, "com.android.messaging") == 0 || strcmp(app_id, "com.google.android.apps.messaging") == 0) return &image_sms_48;
    if (strcasecmp(app_id, "call") == 0 || strcmp(app_id, "com.android.dialer") == 0) return &image_call_48;
    if (strcmp(app_id, "com.google.android.gm") == 0) return &image_gmail_48;
    if (strcmp(app_id, "com.google.android.youtube") == 0) return &image_youtube_48;
    if (strcmp(app_id, "com.whatsapp") == 0) return &image_whatsapp_48;
    if (strcmp(app_id, "com.facebook.orca") == 0) return &image_messenger_48;
    if (strcmp(app_id, "org.telegram.messenger") == 0) return &image_telegram_48;
    if (strcmp(app_id, "com.microsoft.office.outlook") == 0) return &image_outlook_48;
    return &image_notification_48;
}

static void update_card_content(int idx)
{
    if (idx < 0 || idx >= notif_count) return;
    char dt[17];
    format_datetime_ymd_hhmm(notif_buf[idx].ts_iso, dt, sizeof(dt));
    /*if (lbl_num[idx]) {
        char num[8];
        lv_snprintf(num, sizeof(num), "# %d", idx + 1); // Latest is 1
        lv_label_set_text(lbl_num[idx], num);
    }*/
    // Choose friendly app name and avatar by known IDs
    const char* app_id = notif_buf[idx].app;
    const char* friendly = app_id;
    uint32_t color = 0; char letter = 0;
    if (app_id) {
        if (strcasecmp(app_id, "sms") == 0 || strcmp(app_id, "com.android.messaging") == 0 || strcmp(app_id, "com.google.android.apps.messaging") == 0) {
            friendly = "SMS"; color = 0x10B981; letter = 'S';
        } else if (strcasecmp(app_id, "call") == 0 || strcmp(app_id, "com.android.dialer") == 0) {
            friendly = "Call"; color = 0x22C55E; letter = 'C';
        } else if (strcmp(app_id, "com.google.android.gm") == 0) {
            friendly = "Gmail"; color = 0xEF4444; letter = 'G';
        } else if (strcmp(app_id, "com.whatsapp") == 0) {
            friendly = "WhatsApp"; color = 0x25D366; letter = 'W';
        } else if (strcmp(app_id, "com.facebook.orca") == 0) {
            friendly = "Messenger"; color = 0x0084FF; letter = 'M';
        } else if (strcmp(app_id, "org.telegram.messenger") == 0) {
            friendly = "Telegram"; color = 0x229ED9; letter = 'T';
        } else if (strcmp(app_id, "com.microsoft.office.outlook") == 0) {
            friendly = "Outlook"; color = 0x0078D4; letter = 'O';
        }
    }
    set_label_text(lbl_app, friendly);
    set_label_text(lbl_title, notif_buf[idx].title);
    set_label_text(lbl_message, notif_buf[idx].message);
    set_label_text(lbl_time, dt);

    // Update avatar (image if available; otherwise colored monogram)
    if (avatar && avatar_letter) {
        // Create image holder on first use
        if (!avatar_img) {
            avatar_img = lv_image_create(avatar);
            lv_obj_center(avatar_img);
            lv_obj_add_flag(avatar_img, LV_OBJ_FLAG_HIDDEN);
        }

        // Try compiled icon
        const lv_image_dsc_t* icon = pick_app_icon(app_id);
        bool have_icon = (icon != NULL);
        if (have_icon) {
            lv_image_set_src(avatar_img, icon);
            lv_obj_clear_flag(avatar_img, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(avatar_letter, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(avatar_img, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(avatar_letter, LV_OBJ_FLAG_HIDDEN);
        }
        static const uint32_t palette[] = { 0x3B82F6, 0x10B981, 0xF59E0B, 0xEF4444, 0x8B5CF6, 0x06B6D4, 0x22C55E, 0xEAB308 };
        const char *s = app_id && *app_id ? app_id : "?";
        if (color == 0) {
            uint32_t sum = 0; for (const char *p = s; *p; ++p) sum += (uint8_t)(*p);
            color = palette[ sum % (sizeof(palette)/sizeof(palette[0])) ];
        }
        if (letter == 0) {
            for (const char *p = s; *p; ++p) { if (*p != ' ') { letter = *p; break; } }
            if (letter >= 'a' && letter <= 'z') letter = (char)(letter - 'a' + 'A');
            if (letter == 0) letter = '?';
        }
        lv_obj_set_style_bg_color(avatar, lv_color_hex(color), 0);
        if (!have_icon) {
            char txt[2] = { letter, '\0' };
            lv_label_set_text(avatar_letter, txt);
        }
    }
}

static void create_tile(int idx) { (void)idx; }
static void build_single_card(lv_obj_t* parent)
{
    card = lv_obj_create(parent);
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, lv_pct(100), lv_pct(95));
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_set_style_pad_row(card, 10, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_BETWEEN);

    hdr_row = lv_obj_create(card);
    lv_obj_remove_style_all(hdr_row);
    lv_obj_set_size(hdr_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hdr_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    avatar = lv_obj_create(hdr_row);
    lv_obj_remove_style_all(avatar);
    lv_obj_set_size(avatar, 52, 52);
    lv_obj_set_style_radius(avatar, 18, 0);
    lv_obj_set_style_bg_color(avatar, lv_color_hex(0x444444), 0);
    lv_obj_set_style_bg_opa(avatar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(avatar, 0, 0);
    lv_obj_set_style_pad_all(avatar, 0, 0);

    avatar_letter = lv_label_create(avatar);
    lv_obj_center(avatar_letter);
    lv_obj_set_style_text_color(avatar_letter, lv_color_white(), 0);
    lv_obj_set_style_text_font(avatar_letter, &font_bold_42, 0);
    lv_label_set_text(avatar_letter, "N");

    lbl_app = lv_label_create(hdr_row);
    lv_obj_set_style_text_color(lbl_app, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_text_font(lbl_app, &font_normal_26, 0);
    lv_obj_set_style_pad_left(lbl_app, 8, 0);
    lv_label_set_text(lbl_app, "Notifications");
    lv_label_set_long_mode(lbl_app, LV_LABEL_LONG_DOT);

    lbl_title = lv_label_create(card);
    lv_label_set_text(lbl_title, "You don't have new notifications...");
    lv_label_set_long_mode(lbl_title, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xF6F6C2), 0);
    lv_obj_set_style_text_font(lbl_title, &font_bold_26, 0);

    lbl_message = lv_label_create(card);
    lv_label_set_text(lbl_message, "");
    lv_obj_set_width(lbl_message, lv_pct(100));
    lv_label_set_long_mode(lbl_message, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(lbl_message, lv_color_hex(0xE8E8E8), 0);
    lv_obj_set_style_text_font(lbl_message, &font_normal_26, 0);

    lbl_time = lv_label_create(card);
    lv_obj_set_style_pad_bottom(lbl_time, 10, 0);
    lv_obj_set_style_text_color(lbl_time, lv_color_hex(0x909090), 0);
    lv_obj_set_style_text_font(lbl_time, &font_normal_26, 0);
    lv_label_set_text(lbl_time, "");
}

static void update_pager(int active_idx)
{
    if (!lbl_pager) return;
    if (notif_count <= 0) {
        lv_obj_add_flag(lbl_pager, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_clear_flag(lbl_pager, LV_OBJ_FLAG_HIDDEN);
    int pos = active_idx + 1;
    if (pos > notif_count) pos = notif_count;
    char buf[16];
    lv_snprintf(buf, sizeof(buf), "%d/%d", pos, notif_count);
    lv_label_set_text(lbl_pager, buf);
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

static void gesture_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    static lv_point_t press_start = {0,0};
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
        if (dir == LV_DIR_LEFT) {
            if (active_idx + 1 < notif_count) {
                active_idx++;
                update_card_content(active_idx);
                update_pager(active_idx);
            }
        } else if (dir == LV_DIR_RIGHT) {
            if (active_idx > 0) {
                active_idx--;
                update_card_content(active_idx);
                update_pager(active_idx);
            }
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
        if (dx <= -30) { // swipe left
            if (active_idx + 1 < notif_count) {
                active_idx++;
                update_card_content(active_idx);
                update_pager(active_idx);
            }
        } else if (dx >= 30) { // swipe right
            if (active_idx > 0) {
                active_idx--;
                update_card_content(active_idx);
                update_pager(active_idx);
            }
        }
        return;
    }
}

void lv_smartwatch_notifications_create(lv_obj_t * screen)
{
    // Root container (no scroll)
    notif_cont = lv_obj_create(screen);
    lv_obj_remove_style_all(notif_cont);
    lv_obj_set_size(notif_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(notif_cont, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(notif_cont, LV_OPA_COVER, 0);
    lv_obj_clear_flag(notif_cont, LV_OBJ_FLAG_SCROLLABLE);

    // Build one reusable card and enable swipe gestures on container
    lv_obj_add_event_cb(notif_cont, gesture_event_cb, LV_EVENT_GESTURE, NULL);
    lv_obj_add_flag(notif_cont, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_CLICKABLE);
    build_single_card(notif_cont);

    // Ensure gestures on the card bubble up and/or are handled
    //if (card) {
    //    lv_obj_add_flag(card, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_CLICKABLE);
     //   lv_obj_add_event_cb(notif_cont, gesture_event_cb, LV_EVENT_GESTURE, NULL);
    //}

    // Pager indicator at bottom-center
    lbl_pager = lv_label_create(notif_cont);
    lv_obj_set_style_text_color(lbl_pager, lv_color_hex(0x808080), 0);
    lv_obj_set_style_text_font(lbl_pager, &font_normal_26, 0);
    lv_obj_set_align(lbl_pager, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(lbl_pager, -6);
    lv_label_set_text(lbl_pager, "");
    lv_obj_add_flag(lbl_pager, LV_OBJ_FLAG_HIDDEN);
}


void notifications_show(const char* app,
                        const char* title,
                        const char* message,
                        const char* timestamp_iso8601)
{
    if (!notif_cont) return;

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
