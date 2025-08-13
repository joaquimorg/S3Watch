#include "watchface.h"
#include "sensors.h"

lv_obj_t *watchface_create(void) {
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Watchface");
    lv_obj_center(label);
    return label;
}
