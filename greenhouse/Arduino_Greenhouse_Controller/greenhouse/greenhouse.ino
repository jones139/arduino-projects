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
void startWatering();
void stopWatering();

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

// Moving average temperature
float avTemp =  0.0;  // Moving average ambient temperature.
float curTemp = 0.0;  // Current temperature
unsigned long lastTempSampleTime = 0;  // time (millis of last temperature sample.

// watering times
float waterRate = 10.0;   // total amount of water required (litres/day).
float nWatering = 5;      // number of times per day to water.
unsigned long lastWateringTime = 0;



int dt = 1;  //loop period (sec)

/**
 * Interrupt handler called on rising input on pin flowPin.
 */
void flowInterruptHandler() {
  flowPulseCount++; 
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
}
///////////////////////////////////////////////////////

void loop() {
  String readString;
  String k,v;

  unsigned long tnow = millis();

  ////////////////////////////////////////////////
  // temperature monitor
  ////////////////////////////////////////////////
  curTemp = resToTemp(countsToRes(analogRead(thermPin)));
  // Check if we need to add this temperature to rolling average.
  if ((tnow-lastTempSampleTime)>samplePeriod*1000) {
    avTemp = decayFac*curTemp + (1.0-decayFac)*avTemp;
    lastTempSampleTime = tnow;
  }


  /////////////////////////////////////////////
  // Check if it is time to start watering
  /////////////////////////////////////////////
  if ((tnow-lastWateringTime)>1000*86400/nWatering) {
    startWatering();
    lastWateringTime = tnow;
  }



  // Test - alternate warning and pump running to test hardware.
  if (pumpStatus == 0) 
    pumpStatus = 1;
  else
    pumpStatus = 0;
    
  if (warnStatus == 0) 
    warnStatus = 1;
  else
    warnStatus = 0;

  digitalWrite(pumpIndicatorPin,pumpStatus);
  digitalWrite(pumpPin,pumpStatus);
  digitalWrite(warnIndicatorPin,warnStatus);

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
    parseCmd(readString, &k,&v);
    Serial.print("parseCmd k=");
    Serial.println(k);
    Serial.print("parseCmd v=");
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
  

  // wait for dt seconds.
  delay(dt * 1000);

}


void startWatering() {
  pumpStatus = 1;
  flowPulseCount = 0;
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

