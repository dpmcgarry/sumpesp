/*
  dizon_sr04t.h - ESP-IDF Native C Driver for SR04T Ultrasonic Distance Sensor
  Don McGarry
*/

#ifndef DIZON_SR04T_H
#define DIZON_SR04T_H
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "driver/gpio.h"

typedef struct 
{
    bool init;
    gpio_num_t trig_gpio;
    gpio_num_t echo_gpio;
    double max_range_cm;
    uint32_t num_samples;
} dizon_sr04t_conf_t;

typedef struct
{
  bool rec_data;
  bool error;
  double distance_cm;
  uint32_t raw;
} dizon_sr04t_data_t;

esp_err_t dizon_sr04t_init(dizon_sr04t_conf_t* conf);
esp_err_t dizon_sr04t_get_data(dizon_sr04t_conf_t* conf, dizon_sr04t_data_t* data);

#endif