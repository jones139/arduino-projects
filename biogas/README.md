Biogas Generator
================

This is the software to control and monitor my prototype biogas generator.

It is a 25 litre sealed fermentation vessel (from Wilkinsons), fitted with a
u-bend bubbler type airlock.
It has about 150W of trace heating cable wrapped around the bottom third of
the vessel (a disassembled underfloor heating mat from Screwfix.

I have fitted two coper wire electrodes into the bubbler airlock that get
submerged when a bubble passes.   This will be used to count the number of
bubbles to give an indication of the gas flow rate.

There is also a gas detector module attached to the outlet of the airlock
to give us an indication of when we are generating flamable gas.

Three DS18B20 temperature sensor ICs are attached to the outside of the fermentation vessel to indicate temperature at three different heights.

I was originally going to use an Arduino as the controller, but I wanted
internet access to monitor it, so I am going to use a Raspberry Pi - I think
I can interface all the equipment to the GPIO pins on the Pi without needing to 
use the Arduino.



