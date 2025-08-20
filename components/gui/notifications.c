#include "notifications.h"
#include "ui_fonts.h"
#include <string.h>

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

// Container and tiles
static lv_obj_t *notif_cont;      // root container (fills panel)
static lv_obj_t *tv;              // inner tileview for swiping
static lv_obj_t *tiles[MAX_NOTIFICATIONS];
static lv_obj_t *cards[MAX_NOTIFICATIONS];
static lv_obj_t *lbl_num[MAX_NOTIFICATIONS];
static lv_obj_t *lbl_app[MAX_NOTIFICATIONS];
static lv_obj_t *lbl_time[MAX_NOTIFICATIONS];
static lv_obj_t *lbl_title[MAX_NOTIFICATIONS];
static lv_obj_t *lbl_message[MAX_NOTIFICATIONS];
static lv_obj_t *lbl_pager;       // bottom-center pager "n/m"

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

static void update_tile_content(int idx)
{
    if (idx < 0 || idx >= notif_count) return;
    char dt[17];
    format_datetime_ymd_hhmm(notif_buf[idx].ts_iso, dt, sizeof(dt));
    /*if (lbl_num[idx]) {
        char num[8];
        lv_snprintf(num, sizeof(num), "# %d", idx + 1); // Latest is 1
        lv_label_set_text(lbl_num[idx], num);
    }*/
    set_label_text(lbl_app[idx], notif_buf[idx].app);
    set_label_text(lbl_title[idx], notif_buf[idx].title);
    set_label_text(lbl_message[idx], notif_buf[idx].message);
    set_label_text(lbl_time[idx], dt);
}

static void create_tile(int idx)
{
    tiles[idx] = lv_tileview_add_tile(tv, idx, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    lv_obj_t* page = tiles[idx];
    lv_obj_set_style_bg_color(page, lv_color_black(), 0);
    lv_obj_set_size(page, lv_pct(100), lv_pct(100));
    lv_obj_set_style_pad_all(page, 16, 0);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);

    // Card container with rounded corners, subtle gradient and shadow
    lv_obj_t* card = lv_obj_create(page);
    cards[idx] = card;
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_radius(card, 18, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_grad_color(card, lv_color_hex(0x101010), 0);
    lv_obj_set_style_bg_grad_dir(card, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(card, 240, 0);
    lv_obj_set_style_shadow_width(card, 16, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(card, 80, 0);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_set_style_pad_row(card, 10, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    /*lbl_num[idx] = lv_label_create(card);
    lv_obj_set_style_text_color(lbl_num[idx], lv_color_hex(0xF0B000), 0);
    lv_obj_set_style_text_font(lbl_num[idx], &lv_font_montserrat_26, 0);
    lv_label_set_text(lbl_num[idx], "");*/

    // Header row: app (left) + time (right)
    lv_obj_t* hdr = lv_obj_create(card);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_size(hdr, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(hdr, 0, 0);

    lbl_app[idx] = lv_label_create(hdr);
    lv_obj_set_style_text_color(lbl_app[idx], lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_text_font(lbl_app[idx], &font_normal_26, 0);
    lv_label_set_text(lbl_app[idx], "");

    lbl_time[idx] = lv_label_create(hdr);
    lv_obj_set_style_text_color(lbl_time[idx], lv_color_hex(0xA0A0A0), 0);
    lv_obj_set_style_text_font(lbl_time[idx], &font_normal_26, 0);
    lv_label_set_text(lbl_time[idx], "");

    // Title (prominent)
    lbl_title[idx] = lv_label_create(card);
    lv_label_set_text(lbl_title[idx], (idx == 0) ? "No notifications" : "");
    lv_label_set_long_mode(lbl_title[idx], LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(lbl_title[idx], lv_color_hex(0xF6F6C2), 0);
    lv_obj_set_style_text_font(lbl_title[idx], &font_bold_26, 0);

    // Message body
    lbl_message[idx] = lv_label_create(card);
    lv_label_set_text(lbl_message[idx], "");
    lv_obj_set_width(lbl_message[idx], lv_pct(100));
    lv_label_set_long_mode(lbl_message[idx], LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(lbl_message[idx], lv_color_hex(0xC8C8C8), 0);
    lv_obj_set_style_text_font(lbl_message[idx], &font_normal_26, 0);
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

static void tv_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED || code == LV_EVENT_SCROLL_END) {
        lv_obj_t* act = lv_tileview_get_tile_act(tv);
        int idx = 0;
        for (int i = 0; i < MAX_NOTIFICATIONS; ++i) {
            if (tiles[i] == act) { idx = i; break; }
        }
        update_pager(idx);
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

    // Horizontal tileview for items
    tv = lv_tileview_create(notif_cont);
    lv_obj_set_size(tv, lv_pct(100), lv_pct(95));
    lv_obj_set_style_bg_color(tv, lv_color_black(), 0);
    lv_obj_set_scrollbar_mode(tv, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(tv, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_add_event_cb(tv, tv_event_cb, LV_EVENT_ALL, NULL);

    for (int i = 0; i < MAX_NOTIFICATIONS; ++i) {
        create_tile(i);
    }

    // Pager indicator at bottom-center (overlay on top of tv)
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
    if (!tv) return;

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

    // Update all visible tiles
    for (int i = 0; i < notif_count; ++i) {
        update_tile_content(i);
    }

    // Jump to latest
    lv_obj_set_tile_id(tv, 0, 0, LV_ANIM_ON);
    update_pager(0);

    // Subtle entrance animation for the newest card
    if (cards[0]) {
        // Start with slight offset + fade
        lv_obj_set_style_translate_y(cards[0], 16, 0);
        lv_obj_set_style_opa(cards[0], 0, 0);

        lv_anim_t a1; lv_anim_init(&a1);
        lv_anim_set_var(&a1, cards[0]);
        lv_anim_set_values(&a1, 16, 0);
        lv_anim_set_time(&a1, 180);
        lv_anim_set_exec_cb(&a1, notif_anim_set_y);
        lv_anim_start(&a1);

        lv_anim_t a2; lv_anim_init(&a2);
        lv_anim_set_var(&a2, cards[0]);
        lv_anim_set_values(&a2, 0, 255);
        lv_anim_set_time(&a2, 220);
        lv_anim_set_exec_cb(&a2, notif_anim_set_opa);
        lv_anim_start(&a2);
    }
}
