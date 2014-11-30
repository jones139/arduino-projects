int defaultSetpoint = 850;
int heaterPin = 13;
int therm = 0;
int therm2 = 1;
int therm3 = 2;
int setpoint = defaultSetpoint;
int switchPin = 3;
int lightPin = 4;
int switchState = 0;                                            
unsigned long startMillis = 0;

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

////////////////////////////////////////////////
  // reset switch
////////////////////////////////////////////////
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
  Serial.print(time-startMillis);
  Serial.print(" ");
  int val = analogRead( therm );
  Serial.print(val);
  int val2 = analogRead( therm2 );
  Serial.print(" ");
  Serial.print(val2);
  int val3 = analogRead( therm3 );
  Serial.print(" ");
  Serial.print(val3);
  Serial.print(" ");
  Serial.print(val3-val2);
  Serial.println();

////////////////////////////////////////////////
  // temperature controller.
////////////////////////////////////////////////
  if (val < setpoint) {
    digitalWrite(heaterPin, HIGH);
  }
  else {
    digitalWrite(heaterPin, LOW);
  }
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
  delay(1000);

}


