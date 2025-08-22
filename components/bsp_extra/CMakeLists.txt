set(SRCS "")
#list(APPEND SRCS
#    "src/bsp_board_extra.c"
#    "src/pcf85063a.c"
#    "src/rtc_lib.c"
#)
set(SRC_DIRS "src")
set(INCLUDE_DIRS "")
list(APPEND INCLUDE_DIRS "include")

foreach(SRC_DIR IN LISTS SRC_DIRS)
    file(GLOB_RECURSE SRC ${SRC_DIR}/*.c)
    list(APPEND SRCS ${SRC})
    #list(APPEND INCLUDE_DIRS ${SRC_DIR}/include)
endforeach()

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    PRIV_REQUIRES esp_timer esp_psram driver ble_sync
)
