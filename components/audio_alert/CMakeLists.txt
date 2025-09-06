idf_component_register(
    SRCS
        "src/audio_alert.c"
        #"third_party/minimp3/minimp3.c"
    INCLUDE_DIRS
        "include"
        #"third_party/minimp3"
    REQUIRES esp32_s3_touch_amoled_2_06 settings
)
