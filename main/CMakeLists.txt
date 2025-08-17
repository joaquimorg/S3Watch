idf_component_register(
    SRCS
        main.cpp
    INCLUDE_DIRS "."
    REQUIRES ble_sync gui sensors settings bsp_extra esp_event
)
