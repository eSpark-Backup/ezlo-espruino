file(GLOB_RECURSE espruino_src "*.c*")

idf_component_register(SRCS "${espruino_src}"
    INCLUDE_DIRS "include"
    REQUIRES bt driver app_update mbedtls mdns lwip)
