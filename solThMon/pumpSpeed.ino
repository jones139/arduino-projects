#include "config.h"

////////////////////////////////////////////////////////
// Return the load factor of the ac device attached to
// input pin PUMP_SPEED_PIN, assuming the load is determined
// by chopping the ac wave form.
// works by collecting NSAMPLES samples of the instantaneous 
// voltage of the pin, and determining the proportion of the 
// samples that are approximately zero volts.
//
#ifdef USE_SIMPLE_PUMP_SPEED_CALC

float getPumpSpeed(void) {
  int val;
  float loadFactor;
  //Serial.println("getPumpSpeed (SIMPLE_PUMP_SPEED_CALC Version)");
  val = analogRead(PUMP_SPEED_PIN);
  loadFactor = val * 0.3 / 480.;
  //Serial.print("loadFactor=");
  //Serial.print(loadFactor);
  //Serial.println(".");
  return(loadFactor);
}

#else

float getPumpSpeed(void) {
  int n;
  int val;
  int nLowSamples = 0;
  float total = 0.0;
  float mean;
  float loadFactor;
  //Serial.println("getPumpSpeed (LoadFactor Calculation Version)");
  for (n=0;n<NSAMPLES;n++) {
    val  = analogRead(PUMP_SPEED_PIN);
    total +=val;
    if (val<5) nLowSamples++;
    //serviceLED();
  }
  mean = total/NSAMPLES;
  loadFactor = 1.0* (NSAMPLES-nLowSamples) / NSAMPLES;
  //Serial.println("getLoadFactor");
  //Serial.print("mean=");
  //Serial.print(mean);
  //Serial.println(".");
  //Serial.print("nLowSamples=");
  //Serial.print(nLowSamples);
  //Serial.println(".");
  return(loadFactor);
}

#endif
