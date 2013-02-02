/**
 *     This file is part of Seizure_Detector.
 *
 *   Seizure_Detector is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Foobar is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright Graham Jones, 2013.
 *
 * Seizure Detector
 * Samples analogue input and converts to frequency spectrum using
 * FFT.   It monitors one frequency channel against a threshold.  If the threshold is
 * exceeded for a given time, a warning beep is sounded.  If the threshold is exceeded
 * for a longer specified period, an alarm sound is recorded.
 * The frequency spectra are logged to the SD card as a CSV file for future analysis.
 *
 * The circuit:
 *  SD card attached to SPI bus as follows:
 *  CS - pin 10
 *  MOSI - pin 11 on Arduino Uno/Duemilanove/Diecimila
 *  MISO - pin 12 on Arduino Uno/Duemilanove/Diecimila
 *  CLK - pin 13 on Arduino Uno/Duemilanove/Diecimila
 *
 * DS1307 Real Time Clock module connected as follows:
 *   Analogue A4 - STA
 *   Analogue A5 - SCL
 *   2k2 pull up resistors between A4 and +5V and A5 and +5V
 *
 * And when it arrives, an ADXL345 accelerometer module, 
 *   also on A4 and A5.
 *
 * ADXL345 code adapted from example by Jens C Brynildsen
 *   (https://github.com/jenschr/Arduino-libraries/blob/master/ADXL345/examples/ADXL345_no_library/BareBones_ADXL345.pde)
 *
 */
#include <stdint.h>
#include <math.h>     // for sqrt function.
#include <ffft.h>
#include <Time.h>
#include <Wire.h>
#include <ADXL345.h>   // Accelerometer
#include <DS1307RTC.h> // Real Time Clock
#include <Fat16.h>     // SD Card
#include <Fat16util.h> // AD Card memory utilities.
#include <memUtils.h>

// General Settings
static int pinNo = 0;   // ADC Channel to capture
static int SD_CS = 10;  // pin number of the chip select for the SD card.
static int AUDIO_ALARM_PIN = 8;  // digital pin for the audio alarm (connect buzzer from this pin to ground.
static int freq = 64;  // sample frequency (Hz) (128 samples = 1 second collection)
#define LOGFN "SEIZURE.CSV"

static int mon_freq_ch = 2;   // monitor channel 2 of the FFT spectrum.
static int mon_thresh  = 5; // Alarm threshold
static unsigned long mon_warn_millis = 3000; // alarm level must continue for this time to raise warning.
static unsigned long mon_alarm_millis = 10000; // alarm level must continue for this time to raise full alarm.
static unsigned long log_millis = 6000;       // SD card logging period.

// Variables for alarms
boolean mon_thresh_exceeded = false;
boolean mon_warn_state = false;
boolean mon_alarm_state = false;
unsigned long mon_thresh_millis = 0;  // time that the alarm threshold was exceeded.
unsigned long last_log_millis = 0;    // time we last wrote data to the SD card

// Buffers for the Fast Fourier Transform code.
volatile byte position = 0;
int16_t capture[FFT_N];       // Capture Buffer
complex_t bfly_buff[FFT_N];
uint16_t spectrum[FFT_N/2];   // Output buffer

// Buffers for the SD card logging.
SdCard sdCard;
Fat16 logfile;

// Buffer for the accelerometer
ADXL345 accel;

// store error strings in flash to save RAM - uses Fat16Util
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

  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if(timeStatus()!= timeSet) 
     PgmPrintln("Unable to sync with the RTC");
  else
     PgmPrintln("RTC has set the system time");      

  // Initialise accelerometer
  accel.powerOn();
  // might want to adjust gains here with accel.setAxisGains

  // Initialise SD Card
  pinMode(SD_CS,OUTPUT);
  if (!sdCard.init()) error("card.init");  
  // initialize a FAT16 volume
  if (!Fat16::init(&sdCard)) error("Fat16::init");
  
  PgmPrintln("card initialized.");
  PgmPrint("FFT_N=");
  Serial.println(FFT_N);
  Serial.println(memoryFree());

  // make a short pip for a warning
 pinMode(AUDIO_ALARM_PIN, OUTPUT);
 tone(AUDIO_ALARM_PIN,200,1000);

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

////////////////////////////////////////////////////////
// Interrupt service routine - called regularly to collect
// data and store in FFT capture buffer.
//
ISR(TIMER1_COMPA_vect)          // timer compare interrupt service routine
{
  int x,y,z;
  // Do nothing if we are at the end of the capture buffer.
  if (position >= FFT_N)
    return;
  capture[position] = analogRead(pinNo);
  //accel.readAccel(&x,&y,&z);
  //capture[position] = (int)sqrt(x*x+y*y+z*z);
  //capture[position] = x+y+z;
  position++;
}

void digitalClockDisplay(){
  // digital clock display of the time
  printDigits(day());
  PgmPrint("/");
  printDigits(month());
  PgmPrint("/");
  Serial.print(year()); 
  PgmPrint(" ");
  printDigits(hour());
  PgmPrint(":");
  printDigits(minute());
  PgmPrint(":");
  printDigits(second());
  //Serial.println(); 
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  if(digits < 10)
    PgmPrint("0");
  Serial.print(digits);
}

//////////////////////////////////////////////////////////
// Main Loop
//
void loop()
{
  // See if the FFT capture buffer is full.
  // If so, process data.
  if (position == FFT_N) {
    // Do FFT Calculation
    fft_input(capture,bfly_buff);
    fft_execute(bfly_buff);
    fft_output(bfly_buff,spectrum);
    // Reset buffer position ready for next data.
    position = 0;
     
    // Write results to serial port.
    digitalClockDisplay();
    PgmPrint(",");
    for (byte i=0; i<FFT_N/2; i++) {
      //Serial.write(spectrum[i]);
      Serial.print(spectrum[i]);
      PgmPrint(",");
    }
    Serial.println();
    Serial.println(memoryFree());
    
    // Decide if we need to raise an alarm (value over threshold for given period)
    if (spectrum[mon_freq_ch]>=mon_thresh) {
       if (!mon_thresh_exceeded) {
         PgmPrintln("new event");
          mon_thresh_exceeded = true;
          mon_thresh_millis = millis();
       } 
       if ((millis() - mon_thresh_millis) >= mon_alarm_millis) {
         mon_alarm_state = true;
         PgmPrintln("****ALARM****");
       } else if ((millis() - mon_thresh_millis) >= mon_warn_millis) {
         mon_warn_state = true;
         PgmPrintln("**warning**");
       } else {
         PgmPrintln("pre-alarm");
       }
    } else {
      if (mon_thresh_exceeded) {
        PgmPrintln("event reset");
        mon_thresh_exceeded = false;
        mon_warn_state = false;
        mon_alarm_state = false; 
        noTone(AUDIO_ALARM_PIN);
      }
    }
    
    // Write to SD card if we are in a warning or alarm state.
    if (mon_thresh_exceeded || (millis()>(last_log_millis +log_millis))) {
      // Write resutls to SD Card
      PgmPrintln("Writing data to log file...");
      Serial.print(mon_thresh_exceeded);
      PgmPrint(",");
      Serial.print(millis());
      PgmPrint(",");
      Serial.print(last_log_millis);
      PgmPrint(",");
      Serial.println(log_millis);
      
      logfile.open(LOGFN,O_CREAT | O_APPEND | O_WRITE);
      if (!logfile.isOpen()) error ("create");
      logfile.print(now());
      if (mon_warn_state)       logfile.print(", warning,");
      else if (mon_alarm_state) logfile.print(", *ALARM*,");
      else                      logfile.print(",        ,");
      
      for (byte i=0;i<FFT_N/2;i++) {
         logfile.print( spectrum[i] );
         logfile.print( ", ");
      }
      logfile.println();
      logfile.close();
      last_log_millis = millis();
    }
    // make a short pip for a warning
    if (mon_warn_state) tone(AUDIO_ALARM_PIN,200,100);
    // continuous noise for alarm
    if (mon_alarm_state) tone(AUDIO_ALARM_PIN,100);
  }
}

void establishContact() {
 while (Serial.available() <= 0) {
      Serial.write('A');   // send a capital A
      delay(300);
  }
}

