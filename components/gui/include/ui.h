#pragma once
#ifdef __cplusplus
extern "C" {
#endif

void ui_init(void);
void ui_task(void *pvParameters);

// Switch to the Messages tile (notifications screen)
void ui_show_messages_tile(void);

#ifdef __cplusplus
}
#endif
