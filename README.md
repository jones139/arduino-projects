arduino-projects
================

A collection of projects for arduino, with associated libraries collected together into one place.

The projects included here are:

solThMon - Solar Thermal Monitor - displays instantaneous power extracted from a solar thermal collector - measures the collector inlet temperature, outlet temperature and water flow rate to deduce the power and display it on an LCD display.

seizure_detector - Monitors output of an accelerometer and looks for activity in a given frequency range, which could be indicative of an epileptic seizure.   Logs acceleration readings and spectra to a SD card for off-line analysis, and raises an alarm if high activity is detected for a specified period.


The libraries included from other projects are:

OneWire by Paul Stoffregen - http://www.arduino.cc/playground/Learning/OneWire

DallasTemperature by Miles Burton - http://milesburton.com/Dallas_Temperature_Control_Library

Fat16 library to access SD cards by William Greiman - http://code.google.com/p/fat16lib/ (I use this rather than the Aruino SD library, because it uses less RAM.
