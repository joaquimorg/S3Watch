idf_component_register(
    SRCS "display_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES lvgl settings esp32_s3_touch_amoled_2_06 nimble-nordic-uart
)
