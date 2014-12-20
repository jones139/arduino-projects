//pins used
int heaterPin = 9;
int therm = 0;
int therm2 = 1;
int therm3 = 2;
int switchPin = 3;
int lightPin = 4;
//temperature setpoint
// 550 is good for testing (around 25degC)
// 850 is around 40degC
int defaultSetpoint = 800; //temperature controlled to setpoint
int setpoint = defaultSetpoint;
//switch
int switchState = 0; //switch resets timer
//timer
unsigned long startMillis = 0; //timer
//proportional controller
int mode = 1; //mode 1 = proportional controller, mode 0 = simple thermostat
int gain = 10; //constant, multiplied by tempDiff to give output
int output = 0; //varies current, 0 = off, 255 = full power



///////////////////////////////////////////////////////

void setup() {                
  Serial.begin(9600);

  pinMode(heaterPin, OUTPUT);
  pinMode(therm, INPUT);
  pinMode(switchPin, INPUT);
  pinMode(lightPin, OUTPUT);
  digitalWrite(switchPin, HIGH);  
}
///////////////////////////////////////////////////////

void loop() {
  int output;
///////////////////////////////////////////////////////
  // reset switch
///////////////////////////////////////////////////////
  switchState = digitalRead(switchPin);
  if (switchState == HIGH) {
    digitalWrite(lightPin, LOW);
  }
  if (switchState == LOW) {
    digitalWrite(lightPin, HIGH);
  }  
/////////////////////////////////////////////////////////
  // read time and thermistor values and write to serial.
/////////////////////////////////////////////////////////
  unsigned long time = millis();
  if (switchState == LOW) {
    startMillis = time;
  }
  


////////////////////////////////////////////////
  // temperature controller.
////////////////////////////////////////////////
 int val = analogRead( therm );
 int val2 = analogRead( therm2 );
 int val3 = analogRead( therm3 );
  if (mode == 0) {  //simple thermostat
    if (val < setpoint) {  
      output = 255;
    }
    else {
      output = 0;
    }
  }
  if (mode == 1) {  //proportional controller
    int tempDiff = setpoint-val;
    output = tempDiff*gain;
    if (output < 0) {
      output = 0;
    }   
    if (output > 255) {
      output = 255;
    }
  }
  analogWrite (heaterPin,output);
  
////////////////////////////////////////////////
  // respond to commands from serial.
////////////////////////////////////////////////
  if (Serial.available())
  {
    char ch = Serial.read();
    if (ch == 'u')
    {
      setpoint = setpoint + 1;
      Serial.print ("setpoint increased to ");
      Serial.println(setpoint);
    } 
    if (ch == 'd')
    {
      setpoint = setpoint - 1;
      Serial.print ("setpoint decreased to ");
      Serial.println(setpoint);
    } 
    if (ch == 'r')
    {
      setpoint = defaultSetpoint;
      Serial.println ("setpoint reset");
    }

  }
  //Serial.print(time-startMillis);
  Serial.print(time+1);
  Serial.print(",");
  Serial.print(val);
  Serial.print(",");
  Serial.print(val2);
  Serial.print(",");
  Serial.print(val3);
  Serial.print(",");
  Serial.print(val3-val2);
  Serial.print(",");
  Serial.print(output);
  Serial.println();
  
  delay(1000);

}

