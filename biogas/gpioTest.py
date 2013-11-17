#!/usr/bin/env python

from time import sleep
import os
import RPi.GPIO as GPIO

GPIO.setmode(GPIO.BCM)
GPIO.setup(17, GPIO.IN)

while True:
    if (GPIO.input(17) == False):
        print("False")
    else:
        print("True")

