idf_component_register(
    SRCS 
    "ui.c" 
    "watchface.c" 
    "notifications.c" 
    "settings_screen.c" 
    "font/font_roboto_serif_regular_164.c" 
    "font/font_roboto_serif_bold_164.c"
    "font/font_inter_regular_28.c"
    "font/font_inter_bold_28.c"
    "font/font_space_grotesk_regular_23.c"
    "font/font_space_grotesk_regular_28.c"
    "font/font_space_grotesk_regular_80.c"
    "icons/image_battery_icon.c"
    "icons/image_bluetooth_icon.c"
    "icons/image_brightness_icon.c"
    "icons/image_flashlight_icon.c"
    "icons/image_mute_icon.c"
    "icons/image_silence_icon.c"
    "icons/image_settings_icon.c"
    "icons/image_health_off.c"
    "icons/image_health_on.c"
    "icons/image_heart_bg_small.c"
    "icons/image_heart_bg_large.c"
    
    INCLUDE_DIRS "include" 
    REQUIRES lvgl sensors settings display_manager
)
