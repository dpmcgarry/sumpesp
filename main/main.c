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
#include "dizon_scan.h"
#include "dizon_wifi.h"
#include "dizon_http.h"
#include "dizon_EmonLib.h"
#include "aws_clientcredential_keys.h"

#include "dizon_sntp.h"
#include "dizon_mqtt.h"

static const char *TAG = "Template App";

// Current Calibration Constant
// 246.9136 is what the math says this should be
// 190 is what I figured out for my setup using an ammeter
// and a portable electric heater
static const double ICALIBRATION = 30.0;

void app_main(void)
{
    uint8_t mac[6] = {0};
    char macstr[13];
    energy_mon emon;
    double Irms;
    char* timestr;
    esp_mqtt_client_handle_t mqtt_client;
    uint32_t free_mem;

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


    esp_efuse_mac_get_default(mac);    
    sprintf(macstr, "%X%X%X%X%X%X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG, "MAC Address: %s", macstr);
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    printf("Doing Wifi init...");
    wifi_init_sta();
    printf("WiFi Init Done!");
    //do_http_get();
    //ESP_LOGI(TAG, "HTTP_GET");
    
    //printf("HTTP DONE");

    init_sntp();
    mqtt_client=mqtt_app_start();
    emon_current(&emon, ADC1_CHANNEL_6, ICALIBRATION);

    while(true) {
        Irms = emon_calcIrms(&emon, 1480);
        printf("Irms: %f \n", Irms);
        timestr = current_iso_utc_time();
        free_mem = esp_get_free_heap_size();
        send_aws_msg(mqtt_client, macstr, timestr, Irms, free_mem);
        ESP_LOGI(TAG, "[APP] Free memory: %d bytes", free_mem);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
