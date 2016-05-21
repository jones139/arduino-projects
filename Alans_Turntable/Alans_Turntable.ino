void setup()
{
pinMode(2,OUTPUT);
pinMode(3,OUTPUT);
}

void loop()
{
  // reverse direction of rotation.
  digitalWrite(3,!(digitalRead(3)));

  // The number in the for loop sets how far it rotates before changing direction.
  for (int I=0;I<10000;I=I+1) {
     digitalWrite(2,HIGH);
     digitalWrite(2,LOW) ;
     // Delay sets the speed - if it is less than 300 the motor stutters and does not move.
     // larger number = slower rotation.
     delayMicroseconds(350);
  }
}
