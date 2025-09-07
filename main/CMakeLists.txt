idf_component_register(
    SRCS
        lwmalloc.c
        main.cpp
    INCLUDE_DIRS "."
    REQUIRES ble_sync gui sensors settings bsp_extra esp_event audio_alert
)

#spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)