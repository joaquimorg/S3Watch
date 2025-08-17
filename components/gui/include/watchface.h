#pragma once
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif

void watchface_create(lv_obj_t * screen);

// Atualiza indicadores de energia (VBUS/Carregamento/Bateria)
void watchface_set_power_state(bool vbus_in, bool charging, int battery_percent);

// Atualiza indicador de estado BLE
void watchface_set_ble_connected(bool connected);

#ifdef __cplusplus
}
#endif
