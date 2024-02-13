#ifndef EZLO_ESPRUINO_BOARD_H
#define EZLO_ESPRUINO_BOARD_H

#include "esp_log.h"
#define BLE_42_FEATURE_SUPPORT false
#define BLE_50_FEATURE_SUPPORT false

#define ESP32 1
#define DEBUG

#define USE_DEBUGGER

#define EMBEDDED 1
#define USE_ESP32 1
#define USE_NET 1
#define USE_GRAPHICS 1
#define USE_CRYPTO 1
#define USE_SHA256 1
#define USE_SHA512 1
#define USE_TLS 1
#define USE_TELNET 1
#define USE_NEOPIXEL 1
#define USE_FILESYSTEM 1
#define USE_FLASHFS 1
#define USE_BLUETOOTH 1
#define USE_AES 1
#define JSVAR_MALLOC
#define USE_FONT_6X8
#define USE_DEBUGGER
#define USE_TAB_COMPLETE
#define USE_HEATSHRINK
#define ESP_PLATFORM
#define USE_NETWORK_JS

#define ESP_STACK_SIZE 25000
// #define BLUETOOTH 0
#define BLUETOOTH
#define MBEDTLS_CIPHER_MODE_CTR
#define MBEDTLS_CIPHER_MODE_CBC
#define MBEDTLS_CIPHER_MODE_CFB

#endif