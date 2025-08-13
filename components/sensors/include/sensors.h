#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

void sensors_init(void);
void sensors_task(void *pvParameters);
uint32_t sensors_get_step_count(void);

#ifdef __cplusplus
}
#endif
