Greenhouse_Controller README
============================

This directory is set up to use the 'ino' arduino command line tool from
 http://inotool.org.

Compile:
	ino build

Upload:
	ino upload

Serial monitor:
       ino serial

Note that the board type and serial port id are specified in ino.ini or can
be overwritten using the -m <board> -p <port> command line parameters such
as:
	ino upload -m uno -p /dev/ttyUSB1

