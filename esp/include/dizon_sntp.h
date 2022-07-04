#ifndef DIZON_SNTP_H
#define DIZON_SNTP_H

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sntp.h"


/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
RTC_DATA_ATTR static int boot_count = 0;

void time_sync_notification_cb(struct timeval *tv);

void init_sntp(void);

char* current_iso_utc_time(void);

#endif