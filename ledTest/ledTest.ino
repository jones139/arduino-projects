/* FTL-655S LED Display */

/*
 * Defines which segments to illuminate for each number
 */
//0b0gfedcba
volatile unsigned char num2segs[]= {
  0b00111111,  // 0
  0b00000110,  // 1
  0b01011011,  // 2
  0b01001111,  // 3
  0b01100110,  // 4
  0b01101101,  // 5 
  0b01111101,  // 6
  0b00000111,  // 7 
  0b01111111,  // 8
  0b01101111   // 9
};

// 1st dimension is digit number (0-2), 2nd is segment a-g
// number is the cathode number (0 or 1 for pin 1 or 2) for that
// segment.
volatile boolean seg2cath[3][7] = {
 {1,1,1,0,0,0,0}, // 1st digit
 {0,0,0,1,1,1,1}, // 2nd digit
 {1,1,1,0,0,0,0}  // 3rd digit
};

// 1st dimension is digit number (0-2), 2nd is segment a-g
// Value is pin number of that segment's LED
volatile unsigned char seg2displayIp[3][7] = {
 {21,19,20,20,18,21,19},
 {15,16,17,17,18,15,16},
 {13,10,12,12, 9,13,10} 
};

volatile unsigned char displayIp2pin[30] = {
  0,        // 0 not used
  0,0,      // 1-2
  0,0,0,0,  // 3-6
  0,0,0,0,  // 7-10
  0,0,0, // 11-13
  9,8,7,6,  // 14-17
  5,4,3,2   // 18-21
};

volatile unsigned char cathNo2pin[2] = {
  10,9
};




int val1 = 0;    // Value of digit 1.
int val2 = 0;    // Value of digit 2.
int val3 = 0;    // Value of digit 3.
int val4 = 0;    // Value of digit 4.
unsigned long int timer;


/* Show Digit */
void showDigit(int digit, int val) {
 int segNo;
 int segs;
 int displayIp, segPinNo, cathNo,cathOnPinNo,cathOffPinNo;
 segs = num2segs[val];
 for(segNo=0;segNo<8;segNo++) {
   if (bitRead(segs,segNo)) {  //do we need to show this segment?
     displayIp = seg2displayIp[digit][segNo];
     cathNo = seg2cath[digit][segNo];
     segPinNo = displayIp2pin[displayIp];
     if (cathNo == 0) {
       cathOnPinNo = cathNo2pin[1];
       cathOffPinNo = cathNo2pin[0];
     } else {
       cathOnPinNo = cathNo2pin[0];
       cathOffPinNo = cathNo2pin[1];
     }
     // Switch on segment
     digitalWrite(segPinNo,HIGH);
     digitalWrite(cathOffPinNo,LOW);
     digitalWrite(cathOnPinNo,HIGH);
     // wait a bit
     delay(1);
     // Switch off segment
     digitalWrite(segPinNo,LOW);
     digitalWrite(cathOnPinNo,HIGH);
     digitalWrite(cathOffPinNo,HIGH);
     
   }
 }
  
}

void serviceLED() {
   showDigit(0,val1); 
   showDigit(1,val2);
//   showDigit(1,val2);
}


void setup() {
  int i;
  Serial.begin(9600);
  Serial.println("LED Test");

  for (i=0;i<sizeof(displayIp2pin);i++) {
    Serial.print("Setting Pin "); 
    Serial.print(displayIp2pin[i]);
    Serial.println(" to output");
    pinMode(displayIp2pin[i],OUTPUT);
  }

  for (i=0;i<sizeof(cathNo2pin);i++) {
    Serial.print("Setting Pin "); 
    Serial.print(cathNo2pin[i]);
    Serial.println(" to output");
    pinMode(cathNo2pin[i],OUTPUT);
  }

  timer = millis();
  val1=0;
}


void loop() {
   serviceLED();
  
   //showDigit(0,val1);
  
   if (millis()-timer>=1000) {
      timer = millis();
      val1++;
      if (val1>9) {
        val1=0; 
        val2++;
      }
      if (val2>9) {
        val2=0;
      }
   }
  
//    for (val=0;val<=9;val++) {
//      Serial.print("val= ");
//      Serial.println(val);
//       displayVal(1,val);  
//       delay(1000);  
//    } 
}




