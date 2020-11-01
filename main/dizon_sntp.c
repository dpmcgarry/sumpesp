#include "dizon_sntp.h"

static const char *TAG = "sntp";
static void initialize_sntp(void);

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void obtain_time(void)
{
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    localtime_r(&now, &timeinfo);
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_init();
}

void init_sntp(void)
{
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    char tbuf[80];
    struct tm *info;
    info = gmtime( &now);
    strftime(tbuf, 80, "%xT%X", info);
    ESP_LOGI(TAG, "Current Time is: %s", tbuf);
    localtime_r(&now, &timeinfo);
    //free(info);
}

char* current_iso_utc_time(void)
{
    time_t now = 0;
    char* buf=malloc(20 * sizeof(char));
    time(&now);
    struct tm *info;
    info = gmtime( &now);
    strftime(buf, 20, "%Y-%m-%dT%H:%M:%SZ", info);
    ESP_LOGI(TAG, "Returning Time: %s", buf);
    return buf;
}