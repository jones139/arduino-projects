/**
 * Seizure Detector
 * Samples analogue input and converts to frequency spectrum using
 * FFT.
 *
 */
#include <stdint.h>
#include <ffft.h>

#define pinNo 0;   // ADC Channel to capture
static int freq = 100;  // sample frequency (Hz)

volatile byte position = 0;
int16_t capture[FFT_N];       // Capture Buffer
complex_t bfly_buff[FFT_N];
uint16_t spectrum[FFT_N/2];   // Output buffer

void setup()
{
  Serial.begin(57600);
  establishContact();
  capture[0]=analogRead(pinNo);
  // initialize timer1 
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 62500/freq;            // compare match register 16MHz/256/freq
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= (1 << CS12);    // 256 prescaler 
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
  interrupts();             // enable all interrupts
}

ISR(TIMER1_COMPA_vect)          // timer compare interrupt service routine
{
  // Do nothing if we are at the end of the capture buffer.
  if (position >= FFT_N)
    return;
  capture[position] = 0;//analogRead(pinNo);
  position++;
}

void loop()
{
   if (position == FFT_N) {
     //fft_input(capture,bfly_buff);
     //fft_execute(bfly_buff);
     //fft_output(bfly_buff,spectrum);
     
     for (byte i=0; i<64; i++) {
       Serial.write(spectrum[i]);
     }
     position = 0;
   }
}

void establishContact() {
 while (Serial.available() <= 0) {
      Serial.write('A');   // send a capital A
      delay(300);
  }
}

int testfun() {
  return(-1);
}

