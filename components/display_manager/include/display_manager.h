#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

void display_manager_init(void);
void display_manager_turn_on(void);
void display_manager_turn_off(void);
bool display_manager_is_on(void);
void display_manager_reset_timer(void);

#ifdef __cplusplus
}
#endif
