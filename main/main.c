#include <string.h>
#include <stdio.h>

#include "nvs.h"
#include "nvs_flash.h"
#include "config.h"
#include "driver/gpio.h"
#include "nvs_storage.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "wifi_interface.h"
#include "http.h"
#include "debug.h"
#include "frozen.h"
#include "qt_serial.h"
#include "factory_info.h"
#include "devices_common.h"
#include "web_provisioning.h"
#include "interface_common.h"
#include "switch_service.h"
#include "gatt_server.h"
#include "dht.h"
#include "mpu6050.h"
#include "esp_task_wdt.h"

#include "wss.h"

/*********************************** MACROS DEFINATION ********************************/
#define TWDT_TIMEOUT_MS 30000
#define TASK_RESET_PERIOD_MS 2000
#define MAIN_DELAY_MS 10000

/******************** Espruino Js ***********************/
#include "jsdevices.h"
#include <jsinteractive.h>
#include "rtosutil.h"
#include "jstimer.h"
#include "jshardwareUart.h"
#include "jshardwareAnalog.h"
#include "jshardwareTimer.h"
#include "jshardwarePWM.h"
#include "jshardwarePulse.h"
#include "jshardwareSpi.h"
#include "jshardwareESP32.h"
#include "jswrap_wifi.h" // jswrap_wifi_restore
#include "jswrapper.h"

#ifdef BLUETOOTH
#include "bluetooth.h"
#include "esp32_gap_func.h"
#include "esp32_gatts_func.h"
#endif

#include "esp_spi_flash.h"
#include "esp_partition.h"
#include "esp_log.h"

#include "jsvar.h"

TaskHandle_t EspruinoTask;
TaskHandle_t UartTask;

extern void *espruino_stackHighPtr; // Name spaced because this has to be a global variable.
                                    // Used in jsuGetFreeStack().

extern void initialise_wifi(void);

static void uartTask(void *data)
{
    ESP_ERROR_CHECK(esp_task_wdt_add(uartTask));
    ESP_ERROR_CHECK(esp_task_wdt_status(uartTask));
    initConsole();
    while (1)
    {
        consoleToEspruino();
        serialToEspruino();
    }
}

static void espruinoTask(void *data)
{
    int heapVars;

    espruino_stackHighPtr = &heapVars; // Ignore the name, 'heapVars' is on the stack!
                                       // I didn't use another variable becaue this function never ends so
                                       // all variables declared here consume stack space that is never freed.

    PWMInit();
    RMTInit();
    SPIChannelsInit();
    initADC(1);
    jshInit(); // Initialize the hardware
    jswHWInit();

    heapVars = (esp_get_free_heap_size() - 40000) / sizeof(JsVar); // calculate space for jsVars

    // Limit number of JsVars to maximum addressable. Can otherwise be
    // breached by builds with modules removed or boards using PSRAM.
    {
        int maxVars = (1 << JSVARREF_BITS) - 1;

        if (heapVars > maxVars)
        {
            heapVars = maxVars;
        }
    }
    ESP_LOGI("Heap", "HeapVars Size %d", heapVars);
    jsvInit(heapVars); // Initialize the variables

    // not sure why this delay is needed?
    vTaskDelay(200 / portTICK_PERIOD_MS);
    jsiInit(true); // Initialize the interactive subsystem
    if (ESP32_Get_NVS_Status(ESP_NETWORK_WIFI))
        jswrap_wifi_restore();
#ifdef BLUETOOTH
    bluetooth_initDeviceName();
#endif

    ESP_ERROR_CHECK(esp_task_wdt_add(EspruinoTask));
    ESP_ERROR_CHECK(esp_task_wdt_status(EspruinoTask));
    while (1)
    {
        // Reset the task and each user periodically
        esp_task_wdt_reset();
        jsiLoop(); // Perform the primary loop processing
        gatts_sendNUSNotificationIfNotEmpty();
    }
}

// memory mapped address of js_code partition in flash.
char *romdata_jscode = 0;

/***********************************************************************/

static void blinky(void *pv);
static void main_task(void *pv)
{
    char url[128];
    s_factory_info_t *factory = factory_info_get_info();

    while (1)
    {
        char *ws_endpoint = NULL;
        struct json_token wss_uri_tok = JSON_INVALID_TOKEN;

        wait_for_wifi_to_connect();

        snprintf(url, 128, "%s/getserver?json=true", factory->cloud_server);
        TRACE_D("Calling Cloud Server api: %s\r\n", url);
        ws_endpoint = http_get_request(url, factory->ssl_private_key, factory->ssl_shared_key, factory->ca_certificate);

        if (ws_endpoint)
        {
            TRACE_D("ws_endpoint: %s\r\n", ws_endpoint);

            if (json_scanf(ws_endpoint, strlen(ws_endpoint), "{uri:%T}", &wss_uri_tok))
            {
                if (wss_uri_tok.len > 8)
                {
                    web_provisioning_init(&wss_uri_tok);
                    break;
                }
            }

            free(ws_endpoint);
        }

        vTaskDelay(2000 / portTICK_RATE_MS);
    }

    vTaskDelete(NULL);
}

void app_main(void)
{

#if !CONFIG_ESP_TASK_WDT_INIT
    // If the TWDT was not initialized automatically on startup, manually intialize it now
    ESP_ERROR_CHECK(esp_task_wdt_init(TWDT_TIMEOUT_MS, false));
    printf("TWDT initialized\n");
#endif
    // gpio_install_isr_service(0);
    // qt_serial_init();
    // factory_info_init();
    // nvs_storage_init();
    // devices_common_init_devices();
    // interface_common_init();
    // switch_service_init();
    // GATT_SERVER_MAIN();

    // wifi_initialize();
    // wifi_connect_from_nvs();

    // esp_log_level_set("*", ESP_LOG_ERROR); // set all components to ERROR level - suppress Wifi Info
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
#ifdef BLUETOOTH
    jsble_init();
#endif
    spi_flash_init();
    timers_Init();
    timer_Init("EspruinoTimer", 0, 0, 0);

    // Map the js_code partition into memory so can be accessed by E.setBootCode("")
    const esp_partition_t *part;
    spi_flash_mmap_handle_t hrom;
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "js_code");
    if (it == 0)
        jsError("Couldn't find js_code partition - update with partition_espruino.bin\n");
    else
    {
        const esp_partition_t *p = esp_partition_get(it);
        err = esp_partition_mmap(p, 0, p->size, SPI_FLASH_MMAP_DATA, (const void **)&romdata_jscode, &hrom);
        if (err != ESP_OK)
            jsError("Couldn't map js_code!\n");
        // The mapping in hrom is never released - as js code can be called at anytime
    }
    esp_partition_iterator_release(it);

#ifdef RTOS
    queues_init();
    tasks_init();
    task_init(espruinoTask, "EspruinoTask", ESP_STACK_SIZE, 5, 0);
    task_init(uartTask, "ConsoleTask", 2200, 20, 0);
#else
    xTaskCreatePinnedToCore(&espruinoTask, "espruinoTask", 10000, NULL, 5, &EspruinoTask, 0);
    xTaskCreatePinnedToCore(&uartTask, "uartTask", 2200, NULL, 20, &UartTask, 0);
#endif

    // xTaskCreate(main_task, "main task", 20 * 1024, NULL, 2, NULL);
    // xTaskCreate(blinky, "blinky", 2048, NULL, 1, NULL);
}

static void blinky(void *pv)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_2),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    uint32_t state = 0;
    // uint32_t count = 0;
    gpio_config(&io_conf);
    // gpio_pad_select_gpio(dht22_pin);

    while (1)
    {
        state ^= 1;
        gpio_set_level(GPIO_NUM_2, state);

        vTaskDelay(1000 / portTICK_RATE_MS);

        // if (count++ > 20)
        {
            // TRACE_D("-----------------------------------------");
            // TRACE_D("esp_get_free_heap_size - %d", esp_get_free_heap_size());
            // TRACE_D("esp_get_minimum_free_heap_size: %u", esp_get_minimum_free_heap_size());
            // TRACE_D("-----------------------------------------");
        }

        // float humidity, temperature;
        // dht_read_float_data(DHT_TYPE_AM2301, dht22_pin, &humidity, &temperature);
        // printf(">>>>>> SN-002 real data -> Humidity: %.02f, Temperature: %.02f <<<<<<\n", humidity, temperature);
    }
}
