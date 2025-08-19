idf_component_register(
    SRCS "ble_sync.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES bt nvs_flash bsp_extra nimble-nordic-uart json sensors esp_event gui display_manager
)
