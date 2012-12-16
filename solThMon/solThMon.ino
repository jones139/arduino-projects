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
#include <LiquidCrystal.h>
#include <Time.h>
#include "config.h"
#include "pumpSpeed.h"

// lcd display
LiquidCrystal lcd(7,6, 5, 4, 3, 2);

// Variables for tracking timers
unsigned long sampleStartMillis;
unsigned long minuteStartMillis;
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

char appMode = DISPLAY_MODE;  // Display or settings mode?


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

// Variables for SD Card files.
boolean useSD = false;

// The simplest memory availability check I could find - from
// http://playground.arduino.cc/Code/AvailableMemory.
int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}



///////////////////////////////////////////////////////////////////////////
// NAME: printAddress
// DESC: Writes the oneWire DeviceAddress provided on the command line to
//       the serial port in hexadecemal.
// HIST: Borrowed from oneWire example code.
//
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

boolean cmpDeviceAddress(DeviceAddress add1, DeviceAddress add2) {
  boolean retVal = TRUE;
  int i;
  for (i=0;i<8;i++)
    if (add1[i]!=add2[i]) retVal = FALSE; 
  return retVal; 
}


////////////////////////////////////////////////////////////////////////////////////////
// NAME: printPowerSerial
// DESC: Writes out details of the power calculation to the serial port.
//       Uses only the parameters provided to the function, not global variables.
// HIST: 13 November 2012   GJ  ORIGINAL VERSION
//
void printPowerSerial(float pumpSpeed,float flowRate,float T1,float T2,float curPower) {
  //P(pumpSpeedStr)="PumpSpeed (%)";
  Serial.print("PumpSpeed (%): ");
  //serialPrintP(pumpSpeedStr);
  Serial.print(pumpSpeed);
  //P(flowRateStr)=", FlowRate (kg/s): ";
  //serialPrintP(flowRateStr);
  Serial.print(", FlowRate (kg/s): ");
  Serial.print(flowRate);
  //P(tempStr)=", Temps: ";
  //serialPrintP(tempStr);
  Serial.print(", Temps: ");
  Serial.print(T1);
  Serial.print(", ");
  Serial.println(T2);
  Serial.print("Power: ");
  Serial.print(curPower);
  Serial.println(" W");
}

////////////////////////////////////////////////////////////////////////////////////////
// NAME: printPowerLCD
// DESC: Writes out details of the power calculation to the LCD display..
//       Uses only the parameters provided to the function, not global variables.
// HIST: 01 December 2012   GJ  ORIGINAL VERSION
//
void printPowerLCD(
  char displayMode,
  float pumpSpeed,
  float flowRate,
  float T1,float T2,
  float curPower,
  float hourMean,
  float dayMean) {
  float kWh;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("T=");
  if (T1>-30)
    lcd.print(T1,1);
  else
    lcd.print("**.*");    
  lcd.print(",");
  if (T1>-30)
    lcd.print(T2,1);
  else
    lcd.print("**.*");
  lcd.print(",W=");
  lcd.print(pumpSpeed*100,0);
  lcd.print("%");
  
  lcd.setCursor(0,1);
  switch(displayMode) {
    case DISPLAY_CURRENT:
      lcd.print("P_CURR=");
      lcd.print(curPower/1000.,1);
      lcd.print("kW");
      break;
    case DISPLAY_HOURLY:
      kWh = hourMean*1./1000.;
      lcd.print("E_HOUR=");
      lcd.print(kWh,1);
      lcd.print("kWh");
      break;
    case DISPLAY_DAILY:
      kWh = dayMean*24./1000.;
      lcd.print("E_DAY=");
      lcd.print(kWh,1);
      lcd.print("kWh");
      break;
  }
  lcd.blink();  
}  


void init_sensors() {
  boolean foundT2 = FALSE;
  boolean foundT1 = FALSE;

  // Start up the temperature measurement library
  sensors.begin();

  // Grab a count of temperature devices on the one-wire bus.
  numberOfDevices = sensors.getDeviceCount();

  // locate devices on the bus
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices.");

  lcd.setCursor(0,1);
  lcd.print("Found ");
  lcd.print(numberOfDevices,DEC);
  lcd.print(" devices");
  lcd.blink();

  // Loop through each device, print out address - we need this so we can find the 
  //   address of a new temperature sensor if we replace one.
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

      //  Serial.print("Setting resolution to ");
      //  Serial.println(TEMPERATURE_PRECISION, DEC);

      // set the resolution to TEMPERATURE_PRECISION bit (Each Dallas/Maxim device is capable of several different resolutions)
      sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);

      Serial.print("Resolution set to: ");
      Serial.print(sensors.getResolution(tempDeviceAddress), DEC); 
      Serial.println();
    }
    else{
      Serial.print("Found ghost device at ");
      Serial.println(i, DEC);
    }
  }

  for(int i=0;i<numberOfDevices; i++)
  {
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i)) { 
      if (cmpDeviceAddress(tempDeviceAddress,(DeviceAddress)T1_ADDRESS)) {
        //Serial.println("Found T1 Sensor");
        foundT1 = TRUE;
      }
      if (cmpDeviceAddress(tempDeviceAddress,(DeviceAddress)T2_ADDRESS)) {
        //Serial.println("Found T2 Sensor");
        foundT2 = TRUE;
      }
    }
  }
  if (!foundT1) {
    Serial.print("**** ERROR - Failed to Find T1 Sensor ");
    printAddress((DeviceAddress)T2_ADDRESS);
    Serial.println(" *");
  }
  if (!foundT2) {
    Serial.print("**** ERROR - Failed to Find T2 Sensor ");
    printAddress((DeviceAddress)T1_ADDRESS);
    Serial.println(" *");
  }


}

//////////////////////////////////////////////////////////////
void setup(void)
{
  // start serial port
  Serial.begin(9600);
  Serial.println("SolThMon");
  //Serial.print("RAM=");
  Serial.println(freeRam());
  //Serial.println(memoryFree());

  lcd.begin(16,2);
  lcd.print("Solar Thermal Monitor");
  lcd.blink();
  

  setTime(00,00,00,12,12,12);
  
  // Set up pushbuttons
  pinMode(PB1, INPUT);
  digitalWrite(PB1, HIGH);
  
  pinMode(PB2, INPUT);
  digitalWrite(PB2,HIGH);
  
  //Serial.print("RAM=");
  //Serial.println(freeRam());
  Serial.println("init sensors...");
  init_sensors();

  Serial.println("Init timers...");
  timer = millis();
  sampleStartMillis = millis();
  minuteStartMillis = sampleStartMillis;
  hourStartMillis = sampleStartMillis;
  dayStartMillis = hourStartMillis;

  // Initialise daily and hourly average calculations.
  dayCount = 0;
  dayPowerTotal = 0;
  hourCount = 0;
  hourPowerTotal = 0;
  prevDayPowerMean = 0;
  prevHourPowerMean = 0.;  

  Serial.println("setup done");
}

/////////////////////////////////////////////////////////////////
void loop_logging(void)
{ 
  // Variables for power calculation.
  static float pumpSpeed = 0.0;
  static float T1=0.0,T2=0.0;
  static float flowRate=0.0;
  static float curPower=0.0;
  static long int displayUpdateMillis = 0;
  static char displayMode = DISPLAY_CURRENT;

  boolean pb1State,pb2State;
  static boolean prevPb1State=HIGH, prevPb2State=HIGH;
  
  pb1State = digitalRead(PB1);
  pb2State = digitalRead(PB2);
  
  //Serial.print(pb1State); Serial.println(prevPb1State);
  // pb1 cycles display mode - current / hourly / daily
  if ((pb1State == LOW) && (prevPb1State == HIGH)) {
    displayMode ++;
    if (displayMode > DISPLAY_DAILY)
      displayMode = DISPLAY_CURRENT;
    Serial.print("displayMode=");
    Serial.println((int)displayMode);
  } 
  prevPb1State = pb1State;

  // Check if it is time to collect a data sample
  if ((millis() - sampleStartMillis) > SAMPLE_MILLIS) {
    //Serial.println("sample...");
    sampleStartMillis = millis();

    if (DEBUG) Serial.print(pb1State);
    if (DEBUG) Serial.println(pb2State);

    //Serial.println("Getting temperatures..");
    sensors.requestTemperatures(); // Send the command to get temperatures
    T1 = sensors.getTempC(t1Address);
    T2 = sensors.getTempC(t2Address);
    //Serial.println("Getting Load Factor...");
    pumpSpeed = getPumpSpeed();
    flowRate = pumpSpeed * M_CAL / 60.0;
    curPower = flowRate * CP * (T2-T1);

    if (DEBUG) printPowerSerial(pumpSpeed,flowRate,T1,T2,curPower);

    hourPowerTotal += curPower;
    dayPowerTotal += curPower;
    hourCount++;
    dayCount++;

    // Check to see if a minute has elapsed
    if ((millis() - minuteStartMillis) > MINUTE_MILLIS) {
      minuteStartMillis = millis();
    }

    // Check to see if an hour has elapsed
    if ((millis() - hourStartMillis) > HOUR_MILLIS) {
      prevHourPowerMean = hourPowerTotal / hourCount;
      hourPowerTotal = 0.;
      hourCount = 0;
      hourStartMillis = millis();
    }

    // Check to see if a day has elapsed.
    if ((millis() - dayStartMillis) > DAY_MILLIS) {
      prevDayPowerMean = dayPowerTotal / dayCount;
      dayPowerTotal = 0.;
      dayCount = 0;
      dayStartMillis = millis();
    }
  }

  // Check if it is time to update the display values.
  if ((millis() - displayUpdateMillis) > DISPLAY_UPDATE_MILLIS) {
    displayUpdateMillis = millis();
    printPowerLCD(displayMode,pumpSpeed,flowRate,T1,T2,curPower,prevHourPowerMean,prevDayPowerMean);

  }

}

void loop_settings() {
}

void loop(void) {
  if (appMode == DISPLAY_MODE)
    loop_logging(); 
  else if (appMode == SETTINGS_MODE)
    loop_settings();
  else
    appMode == DISPLAY_MODE;
}







