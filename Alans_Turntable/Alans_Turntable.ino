/*  Alans_Turntable - Arduino based model railway turntable controller.
 *   Copyright (C) 2017  Graham Jones & Alan Jones
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Electrical Connections
 * Arduino connected to a stepper motor driver - motor controller step input
 *  connected to STEP_PIN, motor controller direction input connected to
 *  DIR_OUT_PIN.
 *  5 off microswitches connecting pins POS_1_PIN to POS_5_PIN respectively
 *  to ground (uses internal pull-up resistors).
 *  1 microswitch connected to MOVE_PIN to manually MOVE_PIN to manually move
 *   turntable, and one connected to DIR_OUT_PIN to select movement direction.
 *  1 broken beam IR sensor connected to HOME_PIN to pull HOME_PIN low when
 *    home position detected.
 */
#include <EEPROM.h>

// Fixed Parameters
int DEBUG = 1;
int STEP_PIN = 2;     // output pin to tell motor controller to step.
int DIR_OUT_PIN = 3;  // output pin to set motor controller direction.
int POS_1_PIN = 7;    // pin to request position 1
int POS_2_PIN = 6;    // pin to request position 2
int POS_3_PIN = 5;    // pin to request position 3
int POS_4_PIN = 4;    // pin to request position 4
int POS_5_PIN = 11;   // pin to request position 5
int HOME_PIN = 8;     // home switch pin.
int MOVE_PIN = 9;     // input from manual move button.
int DIR_IN_PIN = 10;  // input from manual move direction button.
int PULSES_PER_REV = 16384;   // number of pulses to give one revolution of the motor.
                              // guessed - measured 16412, 16442, 16532.

int POS_1 = 1000;
int POS_2 = 2000;
int POS_3 = 3000;
int POS_4 = 4000;
int pos_5 = 5000;

int presets[] = {0,1000,2000,3000,4000,5000};

long POS_REPORT_PERIOD = 1000;

// 0 - just try to go directly to new position.
const int MOVE_MODE_SIMPLE = 0;
// 1 - return to zero in anticlockwise direction before
//  moving so we always approach from same direction.
const int MOVE_MODE_TO_ZERO = 1;
// 1 - move in anticlockwise direction, overshoot required
// position by OVERSHOOT steps, then approach from
// clockwise direction.
const int MOVE_MODE_OVERSHOOT = 2;  
// 3 - move directly to new position, allowing for
// HYSTERESIS steps of backlash in the gearbox.
const int MOVE_MODE_HYSTERESIS = 3; 

//////////////////////////////////////////////////////
// Set the moveMode to handle hysteresis in gearbox //
//////////////////////////////////////////////////////
int moveMode = MOVE_MODE_OVERSHOOT;


int HYSTERESIS = 90;  // Number of steps of backlash in gears - only used for
                      //  MOVE_MODE_HYSTERESIS
int OVERSHOOT = 150;  // amount to overshoot required position, so we can
                      // approach from clockwise direction (only used for
                      //   MOVE_MODE_OVERSHOOT)




// Global Variables
int curPos = 0;  // Current motor position.
long lastPosReportTime = 0;  // last time we reported position to serial monitor.
int programMode = 0;  // 0 = not programming 1-5 = setting preset 1-5.


/**
 * readPresets() - read the presets[] array from eeprom
 */
void readPresets() {
  // Blank eeprom shoudl have 255 in each byte.
  //writePresets();
  if (EEPROM.read(0) == 255 && EEPROM.read(1) == 255) {
    if (DEBUG) Serial.println("EEPROM not initialised");
    writePresets();
    return;
  } else {
    Serial.println("initialising from EEPROM");
    char *b = (char*)presets;
    for (int addr = 0; addr<sizeof(presets);addr++) {
      b[addr] = EEPROM.read(addr);
    }
    if (1) {
       for(int i=0;i<5;i++) {
        Serial.print(presets[i]);
        Serial.print(",");
       }
      Serial.println(); 
    }
  } 
 }
 
 
/**
 * writePresets() - write the presets[] array to eeprom
 */ 
void writePresets() {
  Serial.println("Writing to EEPROM");
  char *b = (char*)presets;
  for (int addr = 0; addr<sizeof(presets);addr++) {
    EEPROM.write(addr,b[addr]);
  } 
    if (1) {
       for(int i=0;i<5;i++) {
        Serial.print(presets[i]);
        Serial.print(",");
       }
      Serial.println(); 
    }

}

/**
* doStep - do a single step of the motor
*/
void doStep(int dir) {
    digitalWrite(DIR_OUT_PIN,dir);
    digitalWrite(STEP_PIN,HIGH);
     digitalWrite(STEP_PIN,LOW) ;
     // Delay sets the speed - if it is less than 300 the motor stutters and does not move.
     // larger number = slower rotation.
     delayMicroseconds(950);  
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

  if (curPos!=pos) {
    switch (moveMode) {
    case MOVE_MODE_SIMPLE:
      if (curPos<pos) dir = 1;
      while(curPos!=pos)
	doStep(dir);
      break;
    case MOVE_MODE_TO_ZERO:
      // go anticlockwise to zero position
      if (curPos<0) dir = 1;
      while(curPos!=0)
	doStep(dir);
      // then go clockwise to required position
      while(curPos!=pos)
	doStep(1);
      break;
    case MOVE_MODE_OVERSHOOT:
      // go to required position, less OVERSHOOT
      if (curPos<(pos-OVERSHOOT)) dir = 1;
      while(curPos!=(pos-OVERSHOOT))
	doStep(dir);
      // then go clockwise to required position
      while(curPos!=pos)
	doStep(1);
      break;
    case MOVE_MODE_HYSTERESIS:
      if (curPos<pos) {
	// then go clockwise to required position
	while(curPos!=pos)
	  doStep(1);
      } else {
	// go anticlockwise to required position, less HYSTERESIS
	while(curPos!=pos-HYSTERESIS)
	  doStep(0);
	break;
      }
    }    
  }
}


/**
* findHome sweeps in the clockwise direction until the home position
* is found , signified by pin HOME_PIN going low.HI
*/
void findHome() {
  if (DEBUG) Serial.println("findHome - scanning");
  while (!digitalRead(HOME_PIN)) {
    doStep(1);
  }
  curPos = 0;
  if (DEBUG) Serial.println("findHome - complete");
}


/** getPresetButtonPressed() - returns the number of the 
 * preset button that is pressed, or 0 if no preset buttons
 * are pressed.  Note, if more than one button is pressed,
 * returns the lowest id of the pressed buttons.
 */
int getPresetButtonPressed() {
  int buttonId = 0;
  if (digitalRead(POS_5_PIN) == 0) {
    if (DEBUG) Serial.println("preset 5 pressed");
    buttonId = 5;
  }
  if (digitalRead(POS_4_PIN) == 0) {
    if (DEBUG) Serial.println("Preset 4 Pressed");
    buttonId = 4;
  }
  if (digitalRead(POS_3_PIN) == 0) {
    if (DEBUG) Serial.println("Preset 3 Pressed");
    buttonId = 3;
  }
  if (digitalRead(POS_2_PIN) == 0) {
    if (DEBUG) Serial.println("Preset 2 Pressed");
    buttonId = 2;
  }
  if (digitalRead(POS_1_PIN) == 0) {
    if (DEBUG) Serial.println("Preset 1 Pressed");
    buttonId = 1;
  }
  return(buttonId);
}

/**
* processButtons() - poll the position of the buttons and
* react accordingly.
*/
void processButtons() {
  // check which preset button is pressed (zero if none pressed)
  int presetButtonId = getPresetButtonPressed();
  // if we have just released the preset button and we were in
  // programming mode, save the new presets.
  if (presetButtonId==0 && programMode!=0) {
    presets[programMode] = curPos;
    // Save presets to EEPROM.
    writePresets();
    // leave programming mode.
    programMode = 0;
  }
  // If we press a preset without the move button, just go to that
  // preset.
  if (presetButtonId!=0 && digitalRead(MOVE_PIN)==1) {
    if (DEBUG) Serial.print("Moving to Preset ");
    if (DEBUG) Serial.print(presetButtonId);
    if (DEBUG) Serial.print(" - pos=");
    if (DEBUG) Serial.println(presets[presetButtonId]);
    gotoPos(presets[presetButtonId]);
  }
  if (digitalRead(MOVE_PIN)==0) {
      doStep(digitalRead(DIR_IN_PIN));
      if (presetButtonId!=0) {
        programMode = presetButtonId;
        if (DEBUG) Serial.println("updating preset value");
        presets[programMode] = curPos;
      }
      delayMicroseconds(1000); 
  }
  
  
  //if (DEBUG) Serial.print("curPos=");
  //if (DEBUG) Serial.println(curPos);
  //delay(1000);
}

void setup() {
  if (1) Serial.begin(9600);
  
  readPresets();

  pinMode(STEP_PIN,OUTPUT);
  pinMode(DIR_OUT_PIN,OUTPUT);
  pinMode(HOME_PIN,INPUT);
  //digitalWrite(HOME_PIN,1);  // set pull up resistors.

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
  pinMode(POS_5_PIN,INPUT);
  digitalWrite(POS_5_PIN,1);
  

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
