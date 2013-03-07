#!/usr/bin/python

import cv
import math

frameSize = (640,480)
fps = 30
#videoFormat = cv.FOURCC('m','j','p','g')
videoFormat = cv.FOURCC('p','i','m','1')
videoLen = 30 # seconds

size = 50 # pixels
ampl = 50 # pixels
freq = 1  # Hz

def main():
    vw = cv.CreateVideoWriter("testcard.mpg",videoFormat, fps,frameSize,0)

    nFrames = videoLen * fps

    for n in range(0,nFrames):
        frame = cv.CreateImage(frameSize,8,1)

        tFrame = 1.0 * n/fps  # time in seconds.
        angVel = freq * (2*math.pi)
        xOff = ampl * math.sin(angVel*tFrame)
        x = int(0.5*frameSize[0] + xOff)
        y = int(0.5*frameSize[1])
        print "Frame Number %d: time=%f   w=%f  Center=(%d,%d)." % (n,tFrame,angVel,x,y)
        cv.Circle(frame, (x,y), size, cv.Scalar(255,1,1), thickness=1, lineType=8, shift=0) 
        cv.WriteFrame(vw,frame)
        

if __name__ == "__main__":
    main()
