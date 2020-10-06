/*
******************************************** 
* Scan.h - Scans for SSIDs and Prints Them *
********************************************
*/

#ifndef SCAN_H_
#define SCAN_H_
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

static void print_auth_mode(int authmode);

static void print_cipher_type(int pairwise_cipher, int group_cipher);

static void wifi_scan(void);

void old_main(void);

#endif