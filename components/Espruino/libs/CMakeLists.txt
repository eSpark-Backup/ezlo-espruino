
file(GLOB_RECURSE SUBFOLDER_SRCS  
    "Bluetooth/*.c*"
    "filesystem/*.c*"
    "js/*.c*"
    "neopixel/*.c*"
    "pixljs/*.c*"
    "trigger/*.c*"
    "compression/*.c*"
    "graphics/*.c*"
    "microbit/*.c*"
    "network/*.c*"
    "puckjs/*.c*"
    "tv/*.c*"
    "banglejs/*.c*"
    "crypto/*.c*"
    "hexbadge/*.c*"
    "misc/*.c*"
    "nordic_thingy/*.c*"
    "tensorflow/*.c*"
    "wio_lte/*.c*")

idf_component_register(SRCS "${SUBFOLDER_SRCS}"
    INCLUDE_DIRS
    REQUIRES )