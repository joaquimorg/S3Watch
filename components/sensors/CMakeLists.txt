idf_component_register(
    SRCS "sensors.c"
    INCLUDE_DIRS "include"
    REQUIRES esp32_s3_touch_amoled_2_06 waveshare__qmi8658 display_manager
)
