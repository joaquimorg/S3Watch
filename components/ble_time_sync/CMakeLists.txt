idf_component_register(
    SRCS "ble_time_sync.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES bt nvs_flash bsp_extra
)