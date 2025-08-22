idf_component_register(
    SRCS 
    "settings.c" 

    INCLUDE_DIRS "include" 
    REQUIRES esp32_s3_touch_amoled_2_06 bsp_extra spiffs json
)
