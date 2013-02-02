/**
 * Seizure Detector
 * Samples analogue input and converts to frequency spectrum using
 * FFT.
 * The circuit:
 *  SD card attached to SPI bus as follows:
 *  CS - pin 10
 *  MOSI - pin 11 on Arduino Uno/Duemilanove/Diecimila
 *  MISO - pin 12 on Arduino Uno/Duemilanove/Diecimila
 *  CLK - pin 13 on Arduino Uno/Duemilanove/Diecimila
 *
 * DA1307 Real Time Clock module connected as follows:
 *   Analogue A4 - STA
 *   Analogue A5 - SCL
 */
#include <stdint.h>
#include <ffft.h>
#include <Time.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <Fat16.h>
#include <Fat16util.h>
#include <memUtils.h>
static int pinNo = 0;   // ADC Channel to capture
static int SD_CS = 10;  // pin number of the chip select for the SD card.
static int freq = 64;  // sample frequency (Hz) (128 samples = 1 second collection)
#define LOGFN "SEIZURE.TXT"

volatile byte position = 0;
int16_t capture[FFT_N];       // Capture Buffer
complex_t bfly_buff[FFT_N];
uint16_t spectrum[FFT_N/2];   // Output buffer

SdCard sdCard;
Fat16 logfile;

// store error strings in flash to save RAM
#define error(s) error_P(PSTR(s))

void error_P(const char* str) {
  PgmPrint("Error: ");
  SerialPrintln_P(str);
  if (sdCard.errorCode) {
    PgmPrint("SD error: ");
    Serial.println(sdCard.errorCode, HEX);
  }
  while(1);
}

void setup()
{
  Serial.begin(9600);
  //establishContact();
  Serial.println(memoryFree());

  // Initialise SD Card
  pinMode(SD_CS,OUTPUT);
  if (!sdCard.init()) error("card.init");  
  // initialize a FAT16 volume
  if (!Fat16::init(&sdCard)) error("Fat16::init");
  
  Serial.println("card initialized.");
  Serial.print("FFT_N=");
  Serial.println(FFT_N);
  Serial.println(memoryFree());

  // initialize timer1 to set sample frequency. 
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
  capture[position] = analogRead(pinNo);
  //Serial.println(capture[position]);
  position++;
}

void loop()
{
  if (position == FFT_N) {
    fft_input(capture,bfly_buff);
    fft_execute(bfly_buff);
    fft_output(bfly_buff,spectrum);
     
    for (byte i=0; i<FFT_N/2; i++) {
      //Serial.write(spectrum[i]);
      Serial.print(spectrum[i]);
      Serial.print(",");
    }
    position = 0;
    Serial.println();
    Serial.println(memoryFree());

    logfile.open(LOGFN,O_CREAT | O_APPEND | O_WRITE);
    if (!logfile.isOpen()) error ("create");
    for (byte i=0;i<FFT_N/2;i++) {
       logfile.print( spectrum[i] );
       logfile.print( ", ");
    }
    logfile.println();
    logfile.close();
    Serial.println(memoryFree());
  }
}

void establishContact() {
 while (Serial.available() <= 0) {
      Serial.write('A');   // send a capital A
      delay(300);
  }
}

