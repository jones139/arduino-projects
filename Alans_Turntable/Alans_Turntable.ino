
// Fixed Parameters
int DEBUG = 1;
int STEP_PIN = 2;     // output pin to tell motor controller to step.
int DIR_OUT_PIN = 3;  // output pin to set motor controller direction.
int POS_1_PIN = 4;    // pin to request position 1
int POS_2_PIN = 5;    // pin to request position 2
int POS_3_PIN = 6;    // pin to request position 3
int POS_4_PIN = 7;    // pin to request position 4
int HOME_PIN = 8;     // home switch pin.
int MOVE_PIN = 9;     // input from manual move button.
int DIR_IN_PIN = 10;  // input from manual move direction button.
int PULSES_PER_REV = 20000;   // number of pulses to give one revolution of the motor.

int POS_1 = 1000;
int POS_2 = 2000;
int POS_3 = 3000;
int POS_4 = 4000;

long POS_REPORT_PERIOD = 1000;

// Global Variables
int curPos = 0;  // Current motor position.
long lastPosReportTime = 0;  // last time we reported position to serial monitor.

/**
* doStep - do a single step of the motor
*/
void doStep(int dir) {
    digitalWrite(DIR_OUT_PIN,dir);
    digitalWrite(STEP_PIN,HIGH);
     digitalWrite(STEP_PIN,LOW) ;
     // Delay sets the speed - if it is less than 300 the motor stutters and does not move.
     // larger number = slower rotation.
     delayMicroseconds(350);  
     if (dir>0) 
       curPos++;
       //if (curPos>PULSES_PER_REV) {
        // curPos = 0;
        // if (DEBUG) Serial.println("positive overflow - setting CurPos to 0");
       //}
     else
       curPos--;
       //if (curPos<0) {
        // curPos = PULSES_PER_REV;
        // if (DEBUG) Serial.println("negative overflow - setting CurPos to PULSES_PER_REV");
       //}
}

/**
* gotoPos moves the motor to position pos.
*/
void gotoPos(int pos) {
  int dir = 0;
  if (curPos<pos) dir = 1;
  
  while(curPos!=pos)
    doStep(dir);
}


/**
* findHome sweeps in the clockwise direction until the home position
* is found , signified by pin HOME_PIN going low.
*/
void findHome() {
  if (DEBUG) Serial.println("findHome - scanning");
  while (digitalRead(HOME_PIN)) {
    doStep(1);
  }
  if (DEBUG) Serial.println("findHome - complete");
}

/**
* processButtons() - poll the position of the buttons and
* react accordingly.
*/
void processButtons() {
  if (digitalRead(MOVE_PIN)==0) {
     doStep(digitalRead(DIR_IN_PIN));
      delayMicroseconds(1000); 
  }  else if (digitalRead(POS_1_PIN) == 0) {
    if (DEBUG) Serial.println("Moving to POS_1");
    gotoPos(POS_1);
    if (DEBUG) Serial.println("Move to POS_1 complete");
  } else if (digitalRead(POS_2_PIN) == 0) {
    if (DEBUG) Serial.println("Moving to POS_2");
    gotoPos(POS_2);
    if (DEBUG) Serial.println("Move to POS_2 complete");
  } else if (digitalRead(POS_3_PIN) == 0) {
    if (DEBUG) Serial.println("Moving to POS_3");
    gotoPos(POS_3);
    if (DEBUG) Serial.println("Move to POS_3 complete");
  } else if (digitalRead(POS_4_PIN) == 0) {
    if (DEBUG) Serial.println("Moving to POS_4");
    gotoPos(POS_4);
    if (DEBUG) Serial.println("Move to POS_4 complete");
  }
  
  
  //if (DEBUG) Serial.print("curPos=");
  //if (DEBUG) Serial.println(curPos);
  //delay(1000);
}

void setup() {
  if (DEBUG) Serial.begin(9600);

  pinMode(STEP_PIN,OUTPUT);
  pinMode(DIR_OUT_PIN,OUTPUT);
  pinMode(HOME_PIN,INPUT);
  digitalWrite(HOME_PIN,1);  // set pull up resistors.

  pinMode(MOVE_PIN,INPUT);
  digitalWrite(MOVE_PIN,1);
  
  pinMode(DIR_IN_PIN,INPUT);
  digitalWrite(DIR_IN_PIN,1);

  pinMode(POS_1_PIN,INPUT);
  digitalWrite(POS_1_PIN,1);
  pinMode(POS_2_PIN,INPUT);
  digitalWrite(POS_2_PIN,1);
  pinMode(POS_3_PIN,INPUT);
  digitalWrite(POS_3_PIN,1);
  pinMode(POS_4_PIN,INPUT);
  digitalWrite(POS_4_PIN,1);

  lastPosReportTime = millis();

  findHome();
}

void loop()
{
  // reverse direction of rotation.
  //if (DEBUG) Serial.println("Reversing Direction");
  //digitalWrite(DIR_OUT_PIN,!(digitalRead(DIR_OUT_PIN)));

  // The number in the for loop sets how far it rotates before changing direction.
  //for (int I=0;I<10000;I=I+1) {
  //  doStep();
  //}
  processButtons();
  
  if (millis()-lastPosReportTime > POS_REPORT_PERIOD) {
     if (DEBUG) Serial.print("curPos=");
     if (DEBUG) Serial.println(curPos);
     lastPosReportTime = millis(); 
  }
}


