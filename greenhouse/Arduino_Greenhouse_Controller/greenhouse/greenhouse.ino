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

//pins used
int thermPin = 0;
int flowPin = 3;
int pumpPin = 4;
int pumpIndicatorPin = 5;
int warnIndicatorPin = 6;
int sounderPin = 9;
int flowInterrupt = 1;  // (interrupt 1 is connected to pin 3)

//temp calculation
#define thermNom  100000  //resistance at 25 degC
#define tempNom 25        //temperature for nomial resistance
#define numSamp 5         //number of samples
#define bCoEff 3950       //beta coefficient of themistor (usually 3000-4000)
#define serRes 100000     //value of other resistor
int samp[5];

// State Variables
// Flow meter pulse counts.
volatile int flowPulseCount = 0;
int pumpStatus = 0;
int warnStatus = 1;


//timer
unsigned long startMillis = 0; //timer


int serialOutput=0; // By default serial output of data is off, unti
                      // 'start' command is issued by computer.
                      
int dt = 1;  //loop period (sec)

String readString;

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

  attachInterrupt(flowInterrupt, flowInterruptHandler,RISING);  
}
///////////////////////////////////////////////////////

void loop() {
  int output;
  String k,v;

  unsigned long time = millis();

  ////////////////////////////////////////////////
  // temperature monitor
  ////////////////////////////////////////////////
  int res1 = analogRead(thermPin);
  float t1 = resToTemp(countsToRes(res1));

  if (pumpStatus = 0) 
    pumpStatus = 1;
  else
    pumpStatus = 0;
    
  if (warnStatus = 0) 
    warnStatus = 1;
  else
    warnStatus = 0;

  digitalWrite(pumpIndicatorPin,pumpStatus);
  digitalWrite(pumpPin,pumpStatus);
  digitalWrite(warnIndicatorPin,warnStatus);

  Serial.println(digitalRead(flowPin));

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
    
    else {
      if (k=="setpoint") {    //change setpoint
        //setpoint = v.toInt();
      }

    }
    
    readString = "";

    }
  

if (serialOutput==1){
  Serial.print("data,");
  Serial.print(time-startMillis);
  Serial.print(",");
  Serial.print(t1);
  Serial.print(",");
  Serial.print(flowPulseCount);
  Serial.print(",");
  Serial.print(pumpStatus);
  Serial.print(",");
  Serial.print(warnStatus);
  Serial.println();
}
  
  delay(dt * 1000);

}





