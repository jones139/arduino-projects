#include <Serial.h>

void setup() {
 Serial.begin(9600);
  Serial.println("Accelerometer_Test"); 
}

void loop() {
   int x,y,z;
  x = analogRead(0);
   y = analogRead(1);
  z = analogRead(2);
 
 Serial.print(x);
  Serial.print(",");
 Serial.print(y);
Serial.print(",");
Serial.println(z);
delay(200);
}
