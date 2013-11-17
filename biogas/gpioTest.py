#!/usr/bin/env python

from time import sleep
import os
import RPi.GPIO as GPIO

bubblePin = 17
bubbleCount = 0

GPIO.setmode(GPIO.BCM)
GPIO.setup(bubblePin, GPIO.IN)

def bubbleDetected(channel):
    print("bubble detected")
    bubbleCount += 1

GPIO.add_event_detect(bubblePin,GPIO.RISING, callback=bubbleDetected)


while True:
#    if (GPIO.input(bubblePin) == False):
#        print("False")
#    else:
#        print("True")
    print("Bubble Count = %d\n" % bubbleCount)
    sleep(1000)
