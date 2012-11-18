/*
 * Arduino Solar Thermal Monitor Configuration - config.h
 *
 * The values in this file are likely to need to be adjusted to
 * configure the software for a particular application.
 * It defines what is connected to which arduino pin,
 * the device addresses of the two digital temperature monitors,
 * calibration factors etc.
 *
 * Graham Jones, 18 November 2012
 *
 */
 
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 13
#define SWITCH_PIN 12    // the digital pin used for the display switch
#define TEMPERATURE_PRECISION 9
#define NSAMPLES 1000     // Number of analog samples to collect for
// load factor calculation.
#define USE_SIMPLE_LOAD_FACTOR_CALC  // Just measure dc input comment
                                // out this line to measure shape of
                                // the waveform rather than voltage.
#define PUMP_SPEED_PIN A0      // Pin connected to AC voltage signal.
// OneWire address of the collector inlet temp sensor
//#define T1_ADDRESS {0x28,0x60,0xcf,0x4a,0x04,0x00,0x00,0xf5}
#define T2_ADDRESS {0x28,0x85,0xad,0x4a,0x04,0x00,0x00,0xa5}
// OneWire address of the collector outlet temp sensor
//#define T2_ADDRESS {0x28,0x96,0xe3,0x4a,0x04,0x00,0x00,0xec}
#define T1_ADDRESS {0x28,0x7b,0x10,0x4b,0x04,0x00,0x00,0xe8}

#define CP 4200    // Specific Heat Capacity of water J/kg/K
#define M_CAL 5.7  // water flow rate at 100% load factor (l/min).

#define SAMPLE_MILLIS 10000   // Period between samples (miliseconds).
#define HOUR_MILLIS 3600000  // number of miliseconds in an hour.
#define DAY_MILLIS 86400000  // number of miliseconds in a day.

#define DISPLAY_UPDATE_MILLIS 100 // how often to update the display values
#define CUR_POW_MILLIS 10 // how long to hold switch before displaying current power
#define FLOW_MILLIS 1000  // how long to hold switch before displaying flow rate
#define T1_MILLIS   2000  // how long to hold switch before displaying T1
#define T2_MILLIS   3000  // how long to hold switch before displaying T1

// Define the arduino digital pin numbers that are connected to the
// FTL-655S LED display pins.  Array values are arduino pin numbers,
// in the order of FTL-655S display input numbers.
#define DISPLAY_IP_2_PIN { \
  0,        /* 0 not used */ \
  0,0,      /* 1-2 */ \
  0,0,0,0,  /* 3-6 */ \
  0,0,0,0,  /* 7-10 */ \
  0,0,0,    /* 11-13 */ \
  9,8,7,6,  /* 14-17 */ \
  5,4,3,2   /* 18-21 */ \
}

#define CATH_NO_2_PIN { \
  10,9 /* first cathode, second cathode */ \
}

