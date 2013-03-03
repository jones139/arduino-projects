#!/usr/bin/python

import cv
import datetime

IMG_STACK_LEN = 60
ANALYSIS_LAYER = 6
window1 = "Current"
window2 = "Oldest"
imgList = []

def preProcessImage(inImg):
    """
    Returns an image, which is a processed version of the input image inImg.
    Currently just converts to gray scale.
    """
    outImg = cv.CreateImage(cv.GetSize(inImg),8,1)
    cv.CvtColor(inImg,outImg,cv.CV_BGR2GRAY)
    for i in range(ANALYSIS_LAYER):
        outImg = doPyrDown(outImg)
    return(outImg)
# End of preProcessImage

def doPyrDown(inImg):
    """
    Returns an image that has been subjected to Gaussian downsampling via pyrDown.
    Returned image is half the size of the original.
    """
    (width,height)= cv.GetSize(inImg)
    outSize = (width/2, height/2)
    outImg = cv.CreateImage(outSize,8,1)
    cv.PyrDown(inImg,outImg,cv.CV_GAUSSIAN_5x5)
    return(outImg)
# end of doPyrDown

def main():
    """
    Main program - controls grabbing images from video stream and loops around each frame.
    """
    camera = cv.CaptureFromFile("rtsp://192.168.1.18/live_mpeg4.sdp")
    if (camera!=None):
        cv.NamedWindow(window1,cv.CV_WINDOW_AUTOSIZE)
        origImg = cv.QueryFrame(camera)
        lastTime = datetime.datetime.now()
        while (origImg):
            imgList.append(preProcessImage(origImg))
            if (len(imgList)>IMG_STACK_LEN):
                imgList.pop(0)  # Remove first item
            timeDiff = (datetime.datetime.now() - lastTime).total_seconds() 
            fps = 1./timeDiff
            print "timeDiff=%f, fps=%f fps" % (timeDiff,fps)

            cv.ShowImage(window1,origImg)
            cv.ShowImage(window2,imgList[0])
            cv.WaitKey(1) # This is very important or ShowImage doesn't work!!
            
            # Now get a new frame ready to start the loop again
            origImg = cv.QueryFrame(camera)
            lastTime = datetime.datetime.now()
        print "no more images..."
    else:
        print "Error - failed to connect to camera"
# End of main()

if __name__ == "__main__":
    main()
