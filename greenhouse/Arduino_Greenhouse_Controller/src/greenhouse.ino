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
*    D5:  Output - LED  (Pump status indicator)
*    D6:  Output - LED  (Warning indicator)
*    D9:  Output - Piezo sounder
*/  

// Declare functions
int parseCmd(String cmdLine, String *key,String *value);
float resToTemp (float rT);
float countsToRes (int c);
int p2v(int p);
void setWateringRate();
void checkWatering(unsigned long tnow);
void startWatering();
void stopWatering();
void setOutputPins();
void handleSerialInput();
void sendSerialData(unsigned long tnow);
void sendSerialSettings();

//Arduino pin connection definitions.
int thermPin = 0;       // Analogue input from thermistor.
int flowPin = 3;        // pulse flow meter input.
int flowInterrupt = 1;  // (interrupt 1 is connected to pin 3)
int pumpPin = 4;        // solid state relay to control pump.
int pumpIndicatorPin = 5;  // LED output to show pump state.
int warnIndicatorPin = 6;  // LED warning indicator.
int sounderPin = 9;        // Piezo sounder.

// Coefficients for conversion of analogue signal from
// thermistor to temperature in degC.
#define thermNom  100000  //resistance (Ohm) at nominal temperature
#define tempNom 25        //temperature (degC) at nominal resistance
#define bCoEff 3950       //beta coefficient of themistor (usually 3000-4000)
#define serRes 100000     //value of series resistor in the potential divider (Ohm)

// Coefficients for flow rate calculation
int mililitresPerPulse = 2;  // Depends on flow meter!


// State Variables
volatile int flowPulseCount = 0; // Flow meter pulse counts.
int pumpStatus = 0;   // Current pump on/off status.
int warnStatus = 0;   // Current warning status.
int serialOutput=0; // By default serial output of data is off, until
                    // 'start' command is issued by computer.
unsigned long startMillis = 0; // start time.
String readString = "";  // bytes read from serial input.

// Moving average calculation coefficients
int decayFac = 100;      // 1000*decay factor in moving average calc.
int samplePeriod = 3600; // temperature sampling period for moving average.
float avTemp =  0.0;  // Moving average ambient temperature.
float curTemp = 0.0;  // Current temperature
unsigned long lastTempSampleTime = 0;  // time (millis of last temperature sample.

// watering times
int baseWaterRate = 10000;   // total amount of water required (mililitres/day).
int baseWaterTemp = 18;   // temperature at which baseWaterRate is applicable.
int waterTempCoef = 1;    // 1000*multiplcation factor to get actual water rate:	        // waterRate = baseWaterRate + baseWaterRate * waterTempCoef*(temp-baseWaterTemp)/1000 
int waterRate = 0;        // actual required water rate (mililitres/day).
int nWatering = 5;      // number of times per day to water.
unsigned long lastWateringTime = 0;


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
int getFlowPulseCount() {
  return (flowPulseCount);
}


///////////////////////////////////////////////////////

void setup() {                
  Serial.begin(9600);
  startMillis = millis();
  pinMode(thermPin, INPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(flowPin, INPUT);
  pinMode(pumpIndicatorPin, OUTPUT);
  pinMode(warnIndicatorPin, OUTPUT);
  pinMode(sounderPin, OUTPUT);

  // set internal pull-up resistors.
  digitalWrite(flowPin, HIGH); 

  // Set flow meter pin to trigger interrupt so we don't miss any pulses.
  attachInterrupt(flowInterrupt, flowInterruptHandler,RISING);  

  // Initialise current and average temperature.
  curTemp = resToTemp(countsToRes(analogRead(thermPin)));
  avTemp = curTemp;
  setWateringRate();

  // Switch everything off initially.
  pumpStatus = 0;
  warnStatus = 0;
  setOutputPins();
}
///////////////////////////////////////////////////////

void loop() {
  unsigned long tnow = millis();

  checkTemperature(tnow);
  setWateringRate();
  checkWatering(tnow);
  setOutputPins();
  handleSerialInput();
  if (serialOutput==1) sendSerialData();


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
  //Serial.print("Tav=");
  //Serial.print(int(avTemp));
  //Serial.print(", bwt=");
  //Serial.print(baseWaterTemp);
  float corr = 1.0 * baseWaterRate * waterTempCoef*(int(avTemp) - baseWaterTemp)/1000;
  //Serial.print(", cor=");
  //Serial.println(corr);
  waterRate = baseWaterRate 
    + corr;
}

/**
 * Read temperature, and set watering requirement based on average temp.
 */
void checkTemperature(unsigned long tnow) {
  curTemp = resToTemp(countsToRes(analogRead(thermPin)));

  // Check if we need to add this temperature to rolling average.
  if ((tnow-lastTempSampleTime)>samplePeriod*1000) {
    // factor of 1000 because decayFac is 1000 * factor.
    avTemp = (decayFac*curTemp + (1000-decayFac)*avTemp)/1000;
    lastTempSampleTime = tnow;
  }
}

/**
 * Check if it is time to start watering
 */
void checkWatering(unsigned long tnow) {
  // If pump is off, check if we need to start it.
  if (pumpStatus==0) {
    // Is it time for next watering?
    if ((tnow-lastWateringTime)>1000*86400/nWatering) {
      pumpStatus = 1;  // start pump.
      resetFlowPulseCount();  // zero flow meter.
      lastWateringTime = tnow;   // set start time to now.
    }
  } else {
    // Is it time to stop watering.
    float curVol = p2v(getFlowPulseCount());
    if (curVol > waterRate) {
      pumpStatus = 0;   // stop pump.
      resetFlowPulseCount();   // zero flow meter (so we can check for syphon
    }
  }
}


void stopWatering() {
  pumpStatus = 0;
  flowPulseCount = 0;

}



/**
 * Parse string cmdLine into a key/value pair, separated by
 * an equals sign (=).   If no equals sign is found, it sets key
 * to be the string pointed to by cmdLine.
 */
int parseCmd(String cmdLine, String *key,String *value) {
  int equalsPos;
  equalsPos = cmdLine.indexOf('=');
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
  rT = (serRes * c) / (1023 - c);
  return (rT);
}

/**
 * convert a resistance of a thermistor in ohms to a temperature
 */
float resToTemp (float rT) {
  float steinhart;
  steinhart = rT / thermNom;  //(R/Ro)
  steinhart = log(steinhart);  //ln(R/Ro)
  steinhart /= bCoEff;  //1/B * ln(R/Ro)
  steinhart += 1.0 / (tempNom + 273.15);  //+(1/To)
  steinhart = 1.0 / steinhart;  //Invert
  steinhart -= 273.15;  //convert to degC
  return (steinhart);
}

/**
 * Convert a number of flow meter pulses p into volume of water in mililitres.
 */
int p2v(int p) {
  return (p*mililitresPerPulse);

}


/**
 * read bytes from serial input if available, and respond accordingly.
 */
void handleSerialInput() {
  String k,v;
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
    if (readString[readString.length()-1]=='\r') {
      Serial.println("Found end of line - processing...");
      readString[readString.length()-1]=0;  // remove carriage return from line.
      parseCmd(readString, &k,&v);
      Serial.print("parseCmd k=");
      Serial.print(k);
      Serial.print(", v=");
      Serial.println(v);
      
      // First check single word commands (no value provided).
      if (v=="") {
	if (k=="start") {
	  serialOutput=1;
	}
	if (k=="stop") {
	  serialOutput=0;
	}
	// Reset flow pulse counter.
	if (k=="reset") {
	  resetFlowPulseCount(); 
	}
	if (k=="set") {
	  sendSerialSettings();
	}
	if (k=="data") {
	  sendSerialData();
	}
	
      }
      // Otherwise check commands with a value provided.
      else {
	if (k=="bwr") {    //change baseWaterRate
	  baseWaterRate = v.toInt();
	}
	if (k=="bwt") {    //change baseWaterTemp
	  baseWaterTemp = v.toInt();
	}
	if (k=="wrc") {    //change waterTempCoef
	  waterTempCoef = v.toInt();
	}
	if (k=="dec") {    //change decayFac
	  decayFac = v.toInt();
	}
	if (k=="spr") {    //change sample period
	  samplePeriod = v.toInt();
	}
	
      }
      readString = "";
    }
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
  Serial.print(",waterRate:");
  Serial.print(waterRate);
  Serial.print(",flowPulseCount:");
  Serial.print(flowPulseCount);
  Serial.print(",pump:");
  Serial.print(pumpStatus);
  Serial.print(",warn:");
  Serial.print(warnStatus);
  Serial.println("}}");
}

void sendSerialSettings() {
  Serial.print("{set:{");
  Serial.print("bwr:");
  Serial.print(baseWaterRate);
  Serial.print(",bwt:");
  Serial.print(baseWaterTemp);
  Serial.print(",wrc:");
  Serial.print(waterTempCoef);
  Serial.print(",dec:");
  Serial.print(decayFac);
  Serial.print(",spr:");
  Serial.print(samplePeriod);
  Serial.println("}}");
}
