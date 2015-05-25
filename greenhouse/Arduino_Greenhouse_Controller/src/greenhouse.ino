/**
* greenhouse controller.
* Arduino programme to monitor temperature, switch irrigation water
* on and off at set intervals, and control the amount of water 
* dispensed at each irrigation event.
* The following connections to the arduino are assumed.
*    A0:  Thermistor (connected as potential divider with 100k resistor)
*    Digital Input
*    D3:  Input - Pulses from flow meter.
*    D4:  Output - Pump Control (solid state relay).
*    D5:  Not connected (faulty on prototype)
*    D6:  Output - LED  (Pump status indicator)
*    D7:  Output - LED  (Warning indicator)
*    D8:  Input - Reset Button (short pin to ground)
*    D9:  Output - Piezo sounder
*/  
#include <avr/eeprom.h>


// Declare functions
int parseCmd(String cmdLine, String *key,String *value);
float resToTemp (float rT);
float countsToRes (int c);
long getFlowPulseCount();
int p2v(long p);
void setWateringRate();
void checkWatering();
void startWatering();
void stopWatering();
void setOutputPins();
void handleSerialInput();
void sendSerialData();
void sendSerialSettings();
boolean isEepromInitialised();

//Arduino pin connection definitions.
int thermPin = 0;       // Analogue input from thermistor.
int soilmPin = 1;       // Analogue input from soil moisture sensor.
int flowPin = 3;        // pulse flow meter input.
int flowInterrupt = 1;  // (interrupt 1 is connected to pin 3)
int pumpPin = 4;        // solid state relay to control pump.
int pumpIndicatorPin = 6;  // LED output to show pump state.
int warnIndicatorPin = 7;  // LED warning indicator.
int resetButtonPin = 8;    // Reset Button.
int sounderPin = 9;        // Piezo sounder.


// State Variables
volatile long flowPulseCount = 0; // Flow meter pulse counts.
long lastFlowPulseCount = 0;  // flow meter pulse count at last main loop iteration.
unsigned long flowFailStartMillis = 0;  // time when we first detected zero
int pumpStatus = 0;   // Current pump on/off status.
int warnStatus = 0;   // Current warning status.
int serialOutput=0; // By default serial output of data is off, until
                    // 'start' command is issued by computer.
unsigned long startMillis = 0; // start time.
String readString = "";  // bytes read from serial input.

float avTemp =  0.0;  // Moving average ambient temperature.
float curTemp = 0.0;  // Current temperature
int soilm  = 0;       // soil moisture reading (counts)
unsigned long lastTempSampleTime = 0;  // time (millis of last temperature sample.
unsigned long lastWateringTime = 0;
int waterRate = 0;        // actual required water rate (mililitres/day).


/**
 * struct settings_t contains the main settings for the application,
 * and defines the default values for them.
 */
struct settings_t {
  // watering times
  long baseWaterRate = 14000;   // total amount of water required (mililitres/day).
  int baseWaterTemp = 18;   // temperature at which baseWaterRate is applicable.
  int waterTempCoef = 1;    // 1000*multiplcation factor to get actual water rate:	        // waterRate = baseWaterRate + baseWaterRate * waterTempCoef*(temp-baseWaterTemp)/1000 
  int nWatering = 24;      // number of times per day to water.
  int pulseWarnThresh = 20;  // Number of flow meter pulses required to 
  // generate warning when pump is switched off.
  unsigned long flowFailTimeout = 10;  // Number of seconds for which zero flow rate is
                            // recorded before the system trips with a flow fail                            // fault.
  // Moving average calculation coefficients
  int decayFac = 100;      // 1000*decay factor in moving average calc.
  long samplePeriod = 3600; // temperature sampling period for moving average.

  // Coefficients for flow rate calculation
  long pulsesPerLitre = 3975;  // Depends on flow meter!

  // Coefficients for conversion of analogue signal from
  // thermistor to temperature in degC.
  long thermNom = 100000;  //resistance (Ohm) at nominal temperature
  int tempNom = 25;        //temperature (degC) at nominal resistance
  long bCoEff = 3950;       //beta coefficient of themistor (usually 3000-4000)
  long serRes = 100000;     //value of series resistor in the potential divider (Ohm)
};

struct settings_t set;
struct settings_t set_default;

// Main loop period (mili-seconds)
int dt = 1000;  //loop period (sec)

/**
 * Interrupt handler called on rising input on pin flowPin.
 */
void flowInterruptHandler() {
  flowPulseCount++; 
}

/**
 * Set the flow integrator to zero.
 * FIXME - may need to disable interrupts while we do this.
 */
void resetFlowPulseCount() {
  flowPulseCount = 0;
}

/**
 * return the current flow meter pulse count.
 * FIXME - may need to disable interrupts while we do this.
 */
long getFlowPulseCount() {
  return (flowPulseCount);
}


///////////////////////////////////////////////////////

void setup() {                
  Serial.begin(9600);
  startMillis = millis();

  if (isEepromInitialised()) {
    Serial.println("eeprom initialised - reading settings");
    readSettings();   
  } else {
    Serial.println("eeprom not initialised - saving default settings");
    saveSettings();
  }
  

  pinMode(thermPin, INPUT);
  pinMode(soilmPin, INPUT);
  pinMode(resetButtonPin, INPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(flowPin, INPUT);
  pinMode(pumpIndicatorPin, OUTPUT);
  pinMode(warnIndicatorPin, OUTPUT);
  pinMode(sounderPin, OUTPUT);

  // set internal pull-up resistors.
  digitalWrite(flowPin, HIGH); 
  digitalWrite(resetButtonPin, HIGH); 

  // Set flow meter pin to trigger interrupt so we don't miss any pulses.
  attachInterrupt(flowInterrupt, flowInterruptHandler,RISING);  

  // Initialise current and average temperature.
  curTemp = resToTemp(countsToRes(analogRead(thermPin)));
  avTemp = curTemp;
  setWateringRate();

  // Set up flow fail monitoring variables.
  lastFlowPulseCount = 0;
  flowFailStartMillis = 0;  // Zero means we are not timing out.

  // Switch everything off initially.
  pumpStatus = 0;
  warnStatus = 0;
  setOutputPins();
}
///////////////////////////////////////////////////////

/**
 * Main loop - repeats indefinitely.
 */
void loop() {

  checkTemperature();
  checkSoilm();
  setWateringRate();
  checkWatering();
  checkFlowFail();
  handleSerialInput();
  checkResetButton();

  // Trip the pump if we have a warning condition.
  if (warnStatus==1)
    pumpStatus = 0;

  if (serialOutput==1) sendSerialData();

  setOutputPins();
  // wait for dt mili-seconds.
  delay(dt);
}


/**
 * Set the output pins to the correct state based on global variables.
 */
void setOutputPins() {
  digitalWrite(pumpIndicatorPin,pumpStatus);
  digitalWrite(pumpPin,pumpStatus);
  digitalWrite(warnIndicatorPin,warnStatus);
}

/**
 * Set watering rate, based on baseline watering rate and average temperature.
 */
void setWateringRate() {
  float corr = 1.0 * set.baseWaterRate * 
    set.waterTempCoef*(int(avTemp) - set.baseWaterTemp)/1000;
  waterRate = set.baseWaterRate + corr;
}

/**
 * Read temperature, and set watering requirement based on average temp.
 */
void checkTemperature() {
  unsigned long tnow = millis();
  int c;
  float r;
  c = analogRead(thermPin);
  //Serial.println(c);
  r = countsToRes(c);
  //Serial.println(r);
  curTemp = resToTemp(r);
  //Serial.println(curTemp);

  // Check if we need to add this temperature to rolling average.
  if ((tnow-lastTempSampleTime)>set.samplePeriod*1000) {
    // factor of 1000 because decayFac is 1000 * factor.
    avTemp = (set.decayFac*curTemp + (1000-set.decayFac)*avTemp)/1000;
    lastTempSampleTime = tnow;
  }
}

/**
 *  Check soil moisture meter reading.
 */
void checkSoilm() {
  soilm = analogRead(soilmPin);
}

/**
 * Check Reset Button - if pressed, reset alarm.
 */
void checkResetButton() {
  int buttonStatus = digitalRead(resetButtonPin);
  if (buttonStatus == 0)
    resetAlarm();
}

/**
 * Check if it is time to start watering
 */
void checkWatering() {
  unsigned long tnow = millis();
  // If pump is off, check if we need to start it.
  if (pumpStatus==0) {
    // Is it time for next watering?
    if ((tnow-lastWateringTime)/1000>86400/set.nWatering) {
      startWatering();
    } else
      // Check for leaks - pulses increasing with pump stopped
      if (getFlowPulseCount() > set.pulseWarnThresh) {
	Serial.print("Leak Detected - flowPulseCount = ");
	Serial.print(getFlowPulseCount());
	Serial.print(", Threshold = ");
	Serial.println(set.pulseWarnThresh);
	raiseAlarm();
    }
  } else {
    // Is it time to stop watering.
    float curVol = p2v(getFlowPulseCount());
    if (curVol > waterRate/set.nWatering) {
      stopWatering();
    }
  }
}


/**
 * Check for failure of the system to deliver flow (pump running but no
 * indicated flow rate.
 * sets warnStatus if flow failure is detected.
 */
void checkFlowFail() {
  long curFlowPulseCount;
  if (pumpStatus == 0) {
    lastFlowPulseCount = 0;
    flowFailStartMillis = 0;
  } else {
    curFlowPulseCount = getFlowPulseCount();
    if (curFlowPulseCount == lastFlowPulseCount) {
      Serial.println("No Flow detected....");
      // flow has not increased since last call of this function?
      if (flowFailStartMillis == 0) {
	Serial.println("Starting timeout...");
        // This is the first call when flow has not increased so start timeout.
	flowFailStartMillis = millis();
      } else {
	Serial.println("Timing out...");
	Serial.print("millis = ");
	Serial.print(millis());
	Serial.print(", flowFailStartMillis = ");
	Serial.print(flowFailStartMillis);
	Serial.print(" timeout = ");
	Serial.print(set.flowFailTimeout);
	Serial.println();
	// we are already timing out so check if timeout has expired
	if ((millis() - flowFailStartMillis) > set.flowFailTimeout*1000) {
	  Serial.println("Timed out - Tripping pump");
	  // Timeout exceeded - raise alarm and stop pump.
	  raiseAlarm();
	  pumpStatus = 0;
	}
      }
    }
  }
  lastFlowPulseCount = curFlowPulseCount;

}

/**
 * Start watering - will dispense waterRate mililitres of water.
 */
void startWatering() {
  unsigned long tnow = millis();
  pumpStatus = 1;  // start pump.
  resetFlowPulseCount();  // zero flow meter.
  lastWateringTime = tnow;   // set start time to now.
}

/**
 * stop watering - shutdown pump.
 */
void stopWatering() {
  pumpStatus = 0;   // stop pump.
  resetFlowPulseCount();   // zero flow meter (so we can check for syphon
}



/**
 * Parse string cmdLine into a key/value pair, separated by
 * an equals sign (=).   If no equals sign is found, it sets key
 * to be the string pointed to by cmdLine.
 */
int parseCmd(String cmdLine, String *key,String *value) {
  int equalsPos;
  equalsPos = cmdLine.indexOf('=');
  Serial.print("equalsPos=");
  Serial.println(equalsPos);
  if (equalsPos==-1) {
    *key=cmdLine;
    *value="";
  }
  else {
    *key=cmdLine.substring(0,equalsPos);
    *value=cmdLine.substring(equalsPos+1);
  }
  return(equalsPos);
}

/**
 * Convert ADC counts 'c' to resistance, assuming it is wired
 * as a potential divider with series resistance serRes ohms.
 * (serRes defined as global variable).
 */
float countsToRes (int c) {
  float rT;
  rT = (1.0*set.serRes * c) / (1024-c );  // was 1023-c
  rT = rT;
  return (rT);
}

/**
 * convert a resistance of a thermistor in ohms to a temperature
 */
float resToTemp (float rT) {
  float steinhart;
  steinhart = rT / (1.0*set.thermNom);  //(R/Ro)
  steinhart = log(steinhart);  //ln(R/Ro)
  steinhart /= 1.0*set.bCoEff;  //1/B * ln(R/Ro)
  steinhart += 1.0 / (1.0*set.tempNom + 273.15);  //+(1/To)
  steinhart = 1.0 / steinhart;  //Invert
  steinhart -= 273.15;  //convert to degC
  return (steinhart);
}

/**
 * Convert a number of flow meter pulses p into volume of water in mililitres.
 */
int p2v(long p) {
  return (1000*p/set.pulsesPerLitre);

}



/**
 * Raise the warning alarm.
 */
void raiseAlarm() {
  Serial.println("raiseAlarm");
  warnStatus = 1;
  tone(sounderPin,500);
}

/**
 * reset the warning alarm.
 */
void resetAlarm() {
  resetFlowPulseCount();
  warnStatus = 0;
  noTone(sounderPin);
}

boolean isEepromInitialised() {
  char bytezero;
  eeprom_read_block((void*)&bytezero,(void*)0,sizeof(bytezero));
  if (bytezero == 255)
    return false;
  else
    return true;
}

/**
 * Save settings to EEPROM.   Sets byte 0 to 0 to show that it has been set.
 * then writes the 'set' structure from byte 1.
 */
void saveSettings() {
  // write flag to show eeprom is initialised.
  eeprom_write_block((void*)0,(void*)1,1);
  // write settings.
  eeprom_write_block((void *)&set,(void*)1,sizeof(set));
}

/** 
 * read settings from EEPROM.
 */
void readSettings() {
  eeprom_read_block((void*)&set,(void*)1,sizeof(set));
}



/**
 * read bytes from serial input if available, and respond accordingly.
 */
void handleSerialInput() {
  String k,v;
  boolean changed = false;
  ////////////////////////////////////////////////
  // respond to commands from serial.
  ////////////////////////////////////////////////
  while (Serial.available()) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      readString += c;
    }
  }

  //Serial.println (readString);
  if (readString.length()>0) {
    // wait for carriage return before processing - this works with picocom.
    if (readString[readString.length()-1]=='\r' 
	|| readString[readString.length()-1]=='\n') {
      //Serial.println("Found end of line - processing...");
      // remove carriage return from line.
      readString[readString.length()-1]=0;  
      // and if we have another, remove that too...
      if (readString[readString.length()-2]=='\r' 
	|| readString[readString.length()-2]=='\n') 
	readString[readString.length()-2]=0;

      int i = readString[readString.length()-1];
      Serial.println(i);
 
      parseCmd(readString, &k,&v);
      k.trim();
      v.trim();
      //Serial.print("parseCmd k=");
      //Serial.print(k);
      //Serial.print(", v=");
      //Serial.println(v);
      Serial.println(k);
      /*i=0;
      for (i=0;i<k.length()-1;i++) {
	Serial.print(k[i]);
	Serial.print(":");
	Serial.print((int)k[i]);
	Serial.print(",");
      }
      Serial.println();
      */

      
      // First check single word commands (no value provided).
      if (k=="dataon") {
	Serial.println("Serial Output On");
	serialOutput=1;
      }
      else if (k=="dataoff") {
	Serial.println("Serial Output Off");
	serialOutput=0;
      }
      else if (k=="wateron") {
	Serial.println("Water On");
	startWatering();
      }
      else if (k=="wateroff") {
	Serial.println("Water Off");
	stopWatering();
      }
      else if (k=="reset") {
	Serial.println("Reset Alarm");
	resetAlarm(); 
      }
      else if (k=="set") {
	Serial.println("Get Settings");
	sendSerialSettings();
      }
      else if (k=="data") {
	Serial.println("Get Data");
	sendSerialData();
      }
      else if (k=="defaults") {
	Serial.println("Restore Default Settings");
	set = set_default;
	changed = true;
      }
      else if (k=="fft") {    //change flowFailTimeout
	set.flowFailTimeout = v.toInt();
	changed = true;
	Serial.println("Set flowFailTimeout()");
      }
      else if (k=="ppl") {    //change pulsesPerLitre
	set.pulsesPerLitre = v.toInt();
	changed = true;
	Serial.println("Set pulsesPerLitre");
      }
      else if (k=="bwr") {    //change baseWaterRate
	set.baseWaterRate = v.toInt();
	changed = true;
	Serial.println("Set BaseWaterRate (ml)");
      }
      else if (k=="bwr") {    //change baseWaterRate
	set.baseWaterRate = v.toInt();
	changed = true;
	Serial.println("Set BaseWaterRate (ml)");
      }
      else if (k=="nwa") {    //change nWatering
	set.nWatering = v.toInt();
	changed = true;
	Serial.println("Set nWatering");
      }
      else if (k=="bwt") {    //change baseWaterTemp
	set.baseWaterTemp = v.toInt();
	changed = true;
	Serial.println("Set BaseWaterTemp (degC)");
      }
      else if (k=="wrc") {    //change waterTempCoef
	set.waterTempCoef = v.toInt();
	changed = true;
	Serial.println("Set WaterTempCoef");
      }
      else if (k=="dec") {    //change decayFac
	set.decayFac = v.toInt();
	changed = true;
	Serial.println("Set DecayFac");
      }
      else if (k=="spr") {    //change sample period
	set.samplePeriod = v.toInt();
	changed = true;
	Serial.println("Set SamplePeriod");
      }
      else if (k=="pwt") {    // pulse warning threshold
	set.pulseWarnThresh = v.toInt();
	changed = true;
	Serial.println("PulseWarnThresh");
      }
      else if (k=="srv") {    // Series resistor value (in thermistor circuit).
	set.serRes = v.toInt();
	changed = true;
	Serial.println("serRes");
      }
      else {
	Serial.print("Unrecognised Command: ");
	Serial.println(k);
      }
      readString = "";
    } else {
      Serial.println(readString);
    }
    if (changed) saveSettings();
  }
}

/**
 * send a json string of current data to serial output
 */
void sendSerialData() {
  // Send data to serial output if required.
  unsigned long tnow = millis();
  Serial.print("{data:{time:");
  Serial.print(tnow-startMillis);
  Serial.print(",curTemp:");
  Serial.print(curTemp);
  Serial.print(",avTemp:");
  Serial.print(avTemp);
  Serial.print(",soilm:");
  Serial.print(soilm);
  Serial.print(",waterRate:");
  Serial.print(waterRate);
  Serial.print(",curVol:");
  Serial.print(p2v(getFlowPulseCount()));
  Serial.print(",pump:");
  Serial.print(pumpStatus);
  Serial.print(",warn:");
  Serial.print(warnStatus);
  Serial.println("}}");
}

void sendSerialSettings() {
  Serial.print("{set:{");
  Serial.print("ppl:");
  Serial.print(set.pulsesPerLitre);
  Serial.print(",bwr:");
  Serial.print(set.baseWaterRate);
  Serial.print(",nwa:");
  Serial.print(set.nWatering);
  Serial.print(",bwt:");
  Serial.print(set.baseWaterTemp);
  Serial.print(",wrc:");
  Serial.print(set.waterTempCoef);
  Serial.print(",dec:");
  Serial.print(set.decayFac);
  Serial.print(",spr:");
  Serial.print(set.samplePeriod);
  Serial.print(",srv:");
  Serial.print(set.serRes);
  Serial.print(",fft:");
  Serial.print(set.flowFailTimeout);
  Serial.print(",pwt:");
  Serial.print(set.pulseWarnThresh);
  Serial.println("}}");
}
