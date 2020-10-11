/*
******************************************** 
* Main.c - Main Entrypoint for Application *
********************************************
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "nvs_flash.h"
#include "scan.h"
#include "wifi.h"
#include "http.h"
#include "EmonLib.h"

static const char *TAG = "Template App";

void app_main(void)
{
    printf("Hello world!\n");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    // Using hardcoded WiFi Information for now
    // TODO: Add app-based configuration / provisioning of creds
    // wifi_scan();
    
    // Print chip information
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");


    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    printf("Doing Wifi init...");
    wifi_init_sta();
    printf("WiFi Init Done!");
    do_http_get();
    ESP_LOGI(TAG, "HTTP_GET");
    
    printf("HTTP DONE");

    // Loop delay then reboot
    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
