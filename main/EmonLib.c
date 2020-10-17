/*
  Emon.cpp - ESP-IDF Native C Implementation of Energy Monitor
  Derived from openenergymonitor Arduino Implementation
  by Trystan Lea, April 27 2010 GNU GPL
*/

#include "EmonLib.h"

//--------------------------------------------------------------------------------------
// Sets the pins to be used for voltage and current sensors
//--------------------------------------------------------------------------------------
void emon_voltage(energy_mon* emon, adc1_channel_t _inPinV, double _VCAL, double _PHASECAL)
{
  emon->inPinV = _inPinV;
  emon->VCAL = _VCAL;
  emon->PHASECAL = _PHASECAL;
  emon->offsetV = ADC_COUNTS>>1;
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(emon->inPinV,ADC_ATTEN_DB_0);
}

extern void emon_current(energy_mon* emon, adc1_channel_t _inPinI, double _ICAL)
{
  emon->inPinI = _inPinI;
  emon->ICAL = _ICAL;
  emon->offsetI = ADC_COUNTS>>1;
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(emon->inPinI,ADC_ATTEN_DB_0);
}

//--------------------------------------------------------------------------------------
// emon_calc procedure
// Calculates realPower,apparentPower,powerFactor,Vrms,Irms,kWh increment
// From a sample window of the mains AC voltage and current.
// The Sample window length is defined by the number of half wavelengths or crossings we choose to measure.
//--------------------------------------------------------------------------------------
void emon_calcVI(energy_mon* emon, unsigned int crossings, unsigned int timeout)
{
  unsigned int crossCount = 0;                             //Used to measure number of times threshold is crossed.
  unsigned int numberOfSamples = 0;                        //This is now incremented

  //-------------------------------------------------------------------------------------------------------------------------
  // 1) Waits for the waveform to be close to 'zero' (mid-scale adc) part in sin curve.
  //-------------------------------------------------------------------------------------------------------------------------
  unsigned long start = esp_timer_get_time() / 1000;    //millis()-start makes sure it doesnt get stuck in the loop if there is an error.

  while(1)                                   //the while loop...
  {
    emon->startV = adc1_get_raw(emon->inPinV);    
    //analogRead(emon->inPinV);                    //using the voltage waveform
    if ((emon->startV < (ADC_COUNTS*0.55)) && (emon->startV > (ADC_COUNTS*0.45))) 
    {
      break;  //check its within range
    }
    if ((esp_timer_get_time() / 1000 - start) > timeout)
    {
      break;
    }
  }

  //-------------------------------------------------------------------------------------------------------------------------
  // 2) Main measurement loop
  //-------------------------------------------------------------------------------------------------------------------------
  start = esp_timer_get_time() / 1000;

  while ((crossCount < crossings) && ((esp_timer_get_time() / 1000 - start) < timeout))
  {
    numberOfSamples++;                       //Count number of times looped.
    emon->lastFilteredV = emon->filteredV;               //Used for delay/phase compensation

    //-----------------------------------------------------------------------------
    // A) Read in raw voltage and current samples
    //-----------------------------------------------------------------------------
    emon->sampleV = adc1_get_raw(emon->inPinV);                 //Read in raw voltage signal
    emon->sampleI = adc1_get_raw(emon->inPinI);                 //Read in raw current signal

    //-----------------------------------------------------------------------------
    // B) Apply digital low pass filters to extract the 2.5 V or 1.65 V dc offset,
    //     then subtract this - signal is now centred on 0 counts.
    //-----------------------------------------------------------------------------
    emon->offsetV = emon->offsetV + ((emon->sampleV - emon->offsetV) / ADC_COUNTS);
    emon->filteredV = emon->sampleV - emon->offsetV;
    emon->offsetI = emon->offsetI + ((emon->sampleI - emon->offsetI) / ADC_COUNTS);
    emon->filteredI = emon->sampleI - emon->offsetI;

    //-----------------------------------------------------------------------------
    // C) Root-mean-square method voltage
    //-----------------------------------------------------------------------------
    emon->sqV= emon->filteredV * emon->filteredV;                 //1) square voltage values
    emon->sumV += emon->sqV;                                //2) sum

    //-----------------------------------------------------------------------------
    // D) Root-mean-square method current
    //-----------------------------------------------------------------------------
    emon->sqI = emon->filteredI * emon->filteredI;                //1) square current values
    emon->sumI += emon->sqI;                                //2) sum

    //-----------------------------------------------------------------------------
    // E) Phase calibration
    //-----------------------------------------------------------------------------
    emon->phaseShiftedV = emon->lastFilteredV + emon->PHASECAL * (emon->filteredV - emon->lastFilteredV);

    //-----------------------------------------------------------------------------
    // F) Instantaneous power calc
    //-----------------------------------------------------------------------------
    emon->instP = emon->phaseShiftedV * emon->filteredI;          //Instantaneous Power
    emon->sumP += emon->instP;                               //Sum

    //-----------------------------------------------------------------------------
    // G) Find the number of times the voltage has crossed the initial voltage
    //    - every 2 crosses we will have sampled 1 wavelength
    //    - so this method allows us to sample an integer number of half wavelengths which increases accuracy
    //-----------------------------------------------------------------------------
    emon->lastVCross = emon->checkVCross;
    if (emon->sampleV > emon->startV)
    {
      emon->checkVCross = true;
    }
    else
    {
      emon->checkVCross = false;
    }
    if (numberOfSamples==1)
    {
      emon->lastVCross = emon->checkVCross;
    }
    if (emon->lastVCross != emon->checkVCross)
    {
      crossCount++;
    }
  }

  //-------------------------------------------------------------------------------------------------------------------------
  // 3) Post loop calculations
  //-------------------------------------------------------------------------------------------------------------------------
  //Calculation of the root of the mean of the voltage and current squared (rms)
  //Calibration coefficients applied.

  double V_RATIO = emon->VCAL *((SUPPLY_VOLTAGE/1000.0) / (ADC_COUNTS));
  emon->Vrms = V_RATIO * sqrt(emon->sumV / numberOfSamples);

  double I_RATIO = emon->ICAL *((SUPPLY_VOLTAGE/1000.0) / (ADC_COUNTS));
  emon->Irms = I_RATIO * sqrt(emon->sumI / numberOfSamples);

  //Calculation power values
  emon->realPower = V_RATIO * I_RATIO * emon->sumP / numberOfSamples;
  emon->apparentPower = emon->Vrms * emon->Irms;
  emon->powerFactor=emon->realPower / emon->apparentPower;

  //Reset accumulators
  emon->sumV = 0;
  emon->sumI = 0;
  emon->sumP = 0;
//--------------------------------------------------------------------------------------
}

//--------------------------------------------------------------------------------------
double emon_calcIrms(energy_mon* emon, unsigned int Number_of_Samples)
{
  for (unsigned int n = 0; n < Number_of_Samples; n++)
  {
    emon->sampleI = adc1_get_raw(emon->inPinI);
    //printf("Sample: %d\n", emon->sampleI);
    // Digital low pass filter extracts the 2.5 V or 1.65 V dc offset,
    //  then subtract this - signal is now centered on 0 counts.
    emon->offsetI = (emon->offsetI + (emon->sampleI-emon->offsetI)/ADC_COUNTS);
    
    emon->filteredI = emon->sampleI - emon->offsetI;
    //printf("Filter: %f\n", emon->filteredI);

    // Root-mean-square method current
    // 1) square current values
    emon->sqI = emon->filteredI * emon->filteredI;
    // 2) sum
    emon->sumI += emon->sqI;
  }

  double I_RATIO = emon->ICAL *((SUPPLY_VOLTAGE/1000.0) / (ADC_COUNTS));
  //printf("Ratio: %f\n", I_RATIO);
  emon->Irms = I_RATIO * sqrt(emon->sumI / Number_of_Samples);

  //Reset accumulators
  emon->sumI = 0;
  //--------------------------------------------------------------------------------------

  return emon->Irms;
}

void emon_serialprint(energy_mon* emon)
{
  printf("%f %f %f %f %f \n", emon->realPower, emon->apparentPower, emon->Vrms, 
            emon->Irms, emon->powerFactor);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}