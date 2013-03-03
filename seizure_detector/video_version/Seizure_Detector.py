#!/usr/bin/python

import cv

IMG_STACK_LEN = 60
window1 = "Original"
window2 = "First"
imgList = []

camera = cv.CaptureFromFile("rtsp://192.168.1.18/live_mpeg4.sdp")
if (camera!=None):
    cv.NamedWindow(window1,cv.CV_WINDOW_AUTOSIZE)
    origImg = cv.QueryFrame(camera)
    while (origImg):
        imgList.append(cv.CloneImage(origImg))
        if (len(imgList)>IMG_STACK_LEN):
            imgList.pop(0)  # Remove first item
        print "len(imgList) = %d" % (len(imgList))
        cv.ShowImage(window1,origImg)
        cv.ShowImage(window2,imgList[0])


        origImg = cv.QueryFrame(camera)
        cv.WaitKey(1) # This is very important or ShowImage doesn't work!!
    print "no more images..."
else:
    print "Error - failed to connect to camera"


