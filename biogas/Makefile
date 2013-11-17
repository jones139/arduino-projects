
TARGET       = biogas    # what to call the executable
ARDUINO_LIBS =           # which libraries to include

BOARD_TAG    = uno
ARDUINO_PORT = /dev/ttyACM0 # the tty port for the arduino.
MONITOR_BAUD = 9600      # this should be the value you pass to Serial.begin()

ARDUINO_DIR  = /usr/share/arduino
include $(ARDUINO_DIR)/Arduino.mk

install:
	avrdude -F -V -p ATMEGA328P -c arduino -P$(ARDUINO_PORT)  -U build-$(BOARD_TAG)/$(TARGET).hex

# note - to exit the serial monitor do ctrl-a k (^ak).
monitor:
	screen $(ARDUINO_PORT) $(MONITOR_BAUD)
