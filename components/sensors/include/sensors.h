#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SENSORS_ACTIVITY_IDLE = 0,
    SENSORS_ACTIVITY_WALK,
    SENSORS_ACTIVITY_RUN,
    SENSORS_ACTIVITY_OTHER,
} sensors_activity_t;

void sensors_init(void);
void sensors_task(void *pvParameters);
uint32_t sensors_get_step_count(void);
// Returns current activity classification
sensors_activity_t sensors_get_activity(void);

#ifdef __cplusplus
}
#endif
