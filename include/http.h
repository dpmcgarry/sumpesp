/*
************************************************* 
* http.h - Wrapper for basic HTTP GET Operation *
*************************************************
*/
#ifndef HTTP_H
#define HTTP_H

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"


// Constants for Generic HTTP GET
#define WEB_SERVER "neverssl.com" // Thanks Colm!
#define WEB_PORT "80"
#define WEB_PATH "/"

static const char *REQUEST = "GET " WEB_PATH " HTTP/1.0\r\n"
    "Host: "WEB_SERVER":"WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

static void http_get_task(void *pvParameters);

// Wrapper function to call static function def
void do_http_get();

#endif