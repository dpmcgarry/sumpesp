/*
  sr04t.h - ESP-IDF Native C Driver for SR04T Ultrasonic Distance Sensor
  Don McGarry
*/

#ifndef SR04T_H
#define SR04T_H
#include "driver/gpio.h"

typedef struct 
{
    bool init;
    gpio_num_t trig_gpio;
    gpio_num_t echo_gpio;
    double max_range_cm;
    

} sr04t_conf_t;

#endif