#ifndef DIZON_MQTT_H
#define DIZON_MQTT_H

#include "esp_log.h"
#include "mqtt_client.h"
#include "aws_clientcredential_keys.h"

esp_mqtt_client_handle_t mqtt_app_start(void);

void send_aws_msg(esp_mqtt_client_handle_t client, char* id, char* time, double Irms, uint32_t free_mem);

#endif