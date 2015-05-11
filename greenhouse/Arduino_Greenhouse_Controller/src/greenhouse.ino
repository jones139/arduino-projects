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
float p2v(int p);
void setWateringRate();
void checkWatering(unsigned long tnow);
void startWatering();
void stopWatering();
void setOutputPins();
void handleSerialInput();

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
float litresPerPulse = 0.0022;  // Depends on flow meter!

// Moving average calculation coefficients
#define decayFac 0.1      // decay factor in moving average calc.
#define samplePeriod 3600 // temperature sampling period for moving average.

// State Variables
volatile int flowPulseCount = 0; // Flow meter pulse counts.
int pumpStatus = 0;   // Current pump on/off status.
int warnStatus = 0;   // Current warning status.
int serialOutput=0; // By default serial output of data is off, until
                    // 'start' command is issued by computer.
unsigned long startMillis = 0; // start time.
String readString = "";  // bytes read from serial input.

// Moving average temperature
float avTemp =  0.0;  // Moving average ambient temperature.
float curTemp = 0.0;  // Current temperature
unsigned long lastTempSampleTime = 0;  // time (millis of last temperature sample.

// watering times
float baseWaterRate = 10.0;   // total amount of water required (litres/day).
float baseWaterTemp = 18.0;   // temperature at which baseWaterRate is applicable.
float waterTempCoef = 1.0;    // multiplcation factor to get actual water rate:	            // waterRate = baseWaterRate * waterTempCoef*(temp-baseWaterTemp) 
float waterRate = 0.0;        // actual required water rate (litres/day).
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
  

  // Send data to serial output if required.
  if (serialOutput==1){ 
    Serial.print("data,");
    Serial.print(tnow-startMillis);
    Serial.print(",");
    Serial.print(curTemp);
    Serial.print(",");
    Serial.print(flowPulseCount);
    Serial.print(",");
    Serial.print(pumpStatus);
    Serial.print(",");
    Serial.print(warnStatus);
    Serial.println();
  }
  

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
  waterRate = baseWaterRate * waterTempCoef*(avTemp - baseWaterTemp);
}

/**
 * Read temperature, and set watering requirement based on average temp.
 */
void checkTemperature(unsigned long tnow) {
  curTemp = resToTemp(countsToRes(analogRead(thermPin)));

  // Check if we need to add this temperature to rolling average.
  if ((tnow-lastTempSampleTime)>samplePeriod*1000) {
    avTemp = decayFac*curTemp + (1.0-decayFac)*avTemp;
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
 * Convert a number of flow meter pulses p into volume of water in litres.
 */
float p2v(int p) {
  return (p*litresPerPulse);

}


/**
 * read bytes from serial input if available, and respond accordingly.
 */
void handleSerialInput() {
  String k,v;
  Serial.println("HandleSerialInput()");
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
    Serial.print("lastChar=");
    Serial.println(readString[readString.length()-1]);
    if (readString[readString.length()-1]=='*') {
      Serial.println("Found end of line - processing...");
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
	  flowPulseCount = 0; 
	}
	if (k=="settings") {
	  Serial.print("Set,");
	  Serial.println("xxxxxx");
	}
	
      }
      // Otherwise check commands with a value provided.
      else {
	if (k=="setpoint") {    //change setpoint
	  //setpoint = v.toInt();
	}
	
      }
      readString = "";
    }
  }
}
