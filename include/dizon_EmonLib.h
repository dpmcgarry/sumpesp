/*
  Emon.h - ESP-IDF Native C Implementation of Energy Monitor
  Derived from openenergymonitor Arduino Implementation
  by Trystan Lea, April 27 2010 GNU GPL
*/

#ifndef DIZON_EMONLIB_H
#define DIZON_EMONLIB_H

#include <math.h>
#include <stdio.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/adc.h>

// ESP32 has 12 Bit ADC
#define ADC_BITS    12
#define ADC_COUNTS  (1<<ADC_BITS)

// ESP32 runs at 3.3V
// But the ADC only has a range from 0->1.1V
// So our voltage divider's midpoint is 573mV
// Which makes the ADC range 0->1146
#define SUPPLY_VOLTAGE 1146

typedef struct energy_mon energy_mon;

struct energy_mon
{
  //Useful value variables
  double realPower;
  double apparentPower;
  double powerFactor;
  double Vrms;
  double Irms;
  
  //Set Voltage and current input pins
  adc1_channel_t inPinV;
  adc1_channel_t inPinI;
  //Calibration coefficients
  //These need to be set in order to obtain accurate results
  double VCAL;
  double ICAL;
  double PHASECAL;

  //--------------------------------------------------------------------------------------
  // Variable declaration for emon_calc procedure
  //--------------------------------------------------------------------------------------
  int sampleV;                        //sample_ holds the raw analog read value
  int sampleI;

  double lastFilteredV,filteredV;          //Filtered_ is the raw analog value minus the DC offset
  double filteredI;
  double offsetV;                          //Low-pass filter output
  double offsetI;                          //Low-pass filter output

  double phaseShiftedV;                             //Holds the calibrated phase shifted voltage.

  double sqV,sumV,sqI,sumI,instP,sumP;              //sq = squared, sum = Sum, inst = instantaneous

  int startV;                                       //Instantaneous voltage at start of sample window.

  bool lastVCross, checkVCross;                  //Used to measure number of times threshold is crossed.
};

void emon_voltage(energy_mon* emon, adc1_channel_t _inPinV, double _VCAL, double _PHASECAL);
extern void emon_current(energy_mon* emon, adc1_channel_t _inPinI, double _ICAL);

void emon_calcVI(energy_mon* emon, unsigned int crossings, unsigned int timeout);
double emon_calcIrms(energy_mon* emon, unsigned int NUMBER_OF_SAMPLES);
void emon_print(energy_mon* emon);

#endif
