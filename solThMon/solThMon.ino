/**
 * solThMon - Arduino based solar thermal power monitor.
 * The power being extracted from a solar collector is determined
 * by measuring the temperature difference in the primary circuit
 * water across the solar collector, and estimating the water
 * flow rate.
 * 
 * The collector water inlet and outlet temperature is measured
 * using Dallas 18B20 one wire digital temperature sensors.
 * 
 * The water flow rate is set by a separate solar controller.
 * The pump speed is set by modulating the current to the pump
 * motor.   The load factor (=proportion of full power) of
 * the pump is measured by sampling the motor supply voltage
 * many times (~10000) and checking the proportion of the samples
 * at approximately zero volts.   The actual flow rate is taken to
 * be the load factor multiplied by a calibration factor that
 * has been determined separately by measuring the flow rate
 * using a separate meter.
 *
 * The instantaneous power is then simply P = mCp(T2-T1), where
 * Cp is the specific heat capacity of water (~4.2 kJ/kg/K), m
 * is the mass flow rate (kg/s) and T1 and T2 are the collector
 * inlet and outlet temperatures respectively (K or degC).
 *
 *
 * Copyright Graham Jones, 2012
 *
 */
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 13
#define SWITCH_PIN 12    // the digital pin used for the display switch
#define TEMPERATURE_PRECISION 9
#define NSAMPLES 1000     // Number of analog samples to collect for
// load factor calculation.
#define AC_VOLTS_PIN A0      // Pin connected to AC voltage signal.
// OneWire address of the collector inlet temp sensor
//#define T1_ADDRESS {0x28,0x60,0xcf,0x4a,0x04,0x00,0x00,0xf5}
#define T2_ADDRESS {0x28,0x85,0xad,0x4a,0x04,0x00,0x00,0xa5}
// OneWire address of the collector outlet temp sensor
//#define T2_ADDRESS {0x28,0x96,0xe3,0x4a,0x04,0x00,0x00,0xec}
#define T1_ADDRESS {0x28,0x7b,0x10,0x4b,0x04,0x00,0x00,0xe8}

#define CP 4200    // Specific Heat Capacity of water J/kg/K
#define M_CAL 5.7  // water flow rate at 100% load factor (l/min).

#define SAMPLE_MILLIS 10000   // Period between samples (miliseconds).
#define HOUR_MILLIS 3600000  // number of miliseconds in an hour.
#define DAY_MILLIS 86400000  // number of miliseconds in a day.

// Variables for tracking timers
unsigned long sampleStartMillis;
unsigned long hourStartMillis;
unsigned long dayStartMillis;

// Variables for daily and hourly average calculations.
int hourCount;
int dayCount;
float hourPowerTotal;
float dayPowerTotal;
float prevHourPowerMean;
float prevDayPowerMean;
  int ival1,ival2;


// Initialise oneWire bus 
// with DallasTemperature library, which is used
// for temperature measurement.
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Variables for Temperature Masurement
int numberOfDevices; // Number of temperature devices found
DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address
DeviceAddress t1Address = T1_ADDRESS;
DeviceAddress t2Address = T2_ADDRESS;

unsigned long int timer = 0;


// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

//////////////////////////////////////////////////////////
// function to print the power to serial port
void printPowerSerial(float loadFactor,float flowRate,float T1,float T2,float curPower) {
  Serial.print("Load Factor: ");
  Serial.print(loadFactor);
  Serial.print("FlowRate (kg/s): ");
  Serial.print(flowRate);
  Serial.print(", Temps: ");
  Serial.print(T1);
  Serial.print(", ");
  Serial.println(T2);
  Serial.print("Power: ");
  Serial.print(curPower);
  Serial.println(" W");
}


////////////////////////////////////////////////////////
// Return the load factor of the ac device attached to
// input pin AC_VOLTS_PIN, assuming the load is determined
// by chopping the ac wave form.
// works by collecting NSAMPLES samples of the instantaneous 
// voltage of the pin, and determining the proportion of the 
// samples that are approximately zero volts.
//
float getLoadFactor(void) {
  int n;
  int val;
  int nLowSamples = 0;
  float total = 0.0;
  float mean;
  float loadFactor;
  for (n=0;n<NSAMPLES;n++) {
    val  = analogRead(AC_VOLTS_PIN);
    total +=val;
    if (val<5) nLowSamples++;
    //serviceLED();
  }
  mean = total/NSAMPLES;
  loadFactor = 1.0* (NSAMPLES-nLowSamples) / NSAMPLES;
  Serial.println("getLoadFactor");
  Serial.print("mean=");
  Serial.print(mean);
  Serial.println(".");
  Serial.print("nLowSamples=");
  Serial.print(nLowSamples);
  Serial.println(".");
  return(loadFactor);
}


float getLoadFactor2(void) {
  int val;
  float loadFactor;
  
  val = analogRead(AC_VOLTS_PIN);
  loadFactor = val * 0.3 / 480.;
  Serial.print("loadFactor=");
  Serial.print(loadFactor);
  Serial.println(".");
  return(loadFactor);
}

//////////////////////////////////////////////////////////////
void setup(void)
{
  // start serial port
  Serial.begin(9600);
  Serial.println("SolThMon - Solar Thermal Power Monitor");

  ledInit();
  pinMode(SWITCH_PIN, INPUT);
  digitalWrite(SWITCH_PIN, HIGH);

  // Start up the temperature measurement library
  sensors.begin();

  // Grab a count of temperature devices on the one-wire bus.
  numberOfDevices = sensors.getDeviceCount();

  // locate devices on the bus
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  // Loop through each device, print out address
  for(int i=0;i<numberOfDevices; i++)
  {
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i))
    {
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printAddress(tempDeviceAddress);
      Serial.println();

      Serial.print("Setting resolution to ");
      Serial.println(TEMPERATURE_PRECISION, DEC);

      // set the resolution to TEMPERATURE_PRECISION bit (Each Dallas/Maxim device is capable of several different resolutions)
      sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);

      Serial.print("Resolution actually set to: ");
      Serial.print(sensors.getResolution(tempDeviceAddress), DEC); 
      Serial.println();
    }
    else{
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }
  }
  // TODO - check that the specified devices for T1 and T2 are
  // actually present on the 1 wire bus - print error if not.

  timer = millis();
  sampleStartMillis = millis();
  hourStartMillis = sampleStartMillis;
  dayStartMillis = hourStartMillis;

  // Initialise daily and hourly average calculations.
  dayCount = 0;
  dayPowerTotal = 0;
  hourCount = 0;
  hourPowerTotal = 0;
  prevDayPowerMean = 0;
  prevHourPowerMean = 0.;  

}

/////////////////////////////////////////////////////////////////
void loop(void)
{ 
  // Variables for power calculation.
  float loadFactor;
  float T1,T2;
  float flowRate;
  float curPower;

  if ((millis() - sampleStartMillis) > SAMPLE_MILLIS) {
    Serial.println("sample...");
    sampleStartMillis = millis();

    Serial.println("Getting temperatures..");
    sensors.requestTemperatures(); // Send the command to get temperatures
    T1 = sensors.getTempC(t1Address);
    T2 = sensors.getTempC(t2Address);
    Serial.println("Getting Load Factor...");
    loadFactor = getLoadFactor2();
    flowRate = loadFactor * M_CAL / 60.0;
    curPower = flowRate * CP * (T2-T1);

    printPowerSerial(loadFactor,flowRate,T1,T2,curPower);

    hourPowerTotal += curPower;
    dayPowerTotal += curPower;
    hourCount++;
    dayCount++;

    // Check to see if an hour has elapsed
    if ((millis() - hourStartMillis) > HOUR_MILLIS) {
      prevHourPowerMean = hourPowerTotal / hourCount;
      hourPowerTotal = 0.;
      hourCount = 0;
      hourStartMillis = millis();

      // TODO - write hourly average to SD Card
    }
    // Check to see if a day has elapsed.
    if ((millis() - dayStartMillis) > DAY_MILLIS) {
      prevDayPowerMean = dayPowerTotal / dayCount;
      dayPowerTotal = 0.;
      dayCount = 0;
      dayStartMillis = millis();

      // TODO - write daily average to SD Card
    }
    ival2 = (int)(curPower / 1000);
    ival1 = (int)((curPower - 1000*ival2)/100);
/*    if (millis()-timer>=1000) {
      Serial.println("second timer");
      timer = millis();
      ival1++;
      if (ival1>9) {
        ival1=0; 
        ival2++;
      }
      if (ival2>9) {
        ival2=0;
      }
      
    }
*/
    setLEDVals(ival1,ival2);
  }
  
  // Only service the display if the switch is pressed, to save power.
  if(digitalRead(SWITCH_PIN)==LOW)
    serviceLED();
  //delay(1000);
}




