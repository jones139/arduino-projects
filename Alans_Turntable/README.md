# Alans_Turntable

This program is a simple controller for a model railway turntable.   It uses
a stepper motor driver board that takes a digital signal for the direction
and moves the motor on receipt of pulses from another input.

It expects four digital inputs for buttons to request the turntable to move to
preset positions.

It is possible to control the turntable manually using a switch to set direction
and a button to move it.

On start-up it moves the turntable until another digital input is pulled low
by a reed switch or some other position detector.

All digital inputs are normally high, so the switches should be wired to pull
them down to ground.  No pull-up resistors are required as we make use of the
internal ones in the arduino microcontroller.

The following pins are used:
D2 - output - step signal for motor drive board.
D3 - output - direction signal for motor drive board.
D4 - input - Position 1 button
D5 - input - Position 2 button
D6 - input - Position 3 button
D7 - input - Position 4 button
D8 - input - home position detection.
D9 - input - Manual Move button.
D10 - input - Direction switch for manual move.

The pre-set positions are set in the array presets[], which is stored in
EEPROM so the presets are rememberd following power-off.

if the variable DEBUG is set, then debugging information is written to the serial line.   The current position is written every POS_REPORT_PERIOD milliseconds.

The presets can be changed by pressing and holding a preset button, then using
the manual move button to move the turntable.   The preset is stored when the
preset button is released.


# Usage:
  * Move to a preset location - press and release the preset button corresponding
to the desired lcoation.
  * Move to any location - use the direction switch to select direction of travel
  press the manual move button to move the turntable.
  * Change a preset location.  Press and hold the required presset button.  Press the manual move button until the turntable is in the required position.  Release the preset button to save the new position.