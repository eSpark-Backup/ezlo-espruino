file(GLOB_RECURSE espruino_src "*.c*")

idf_component_register(SRCS "${config_src}"
    INCLUDE_DIRS "../include" "./libs" "../libs/bluetooth" "../libs/compression" "../libs/network/esp32"
    REQUIRES bt driver)