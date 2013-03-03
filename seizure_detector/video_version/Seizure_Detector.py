#!/usr/bin/python

import cv
import datetime

IMG_STACK_LEN = 60
ANALYSIS_LAYER = 6
window1 = "Current"
window2 = "Oldest"
window3 = "Time Data"
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

def getSpectra(imgList):
    """ Calculates the fourier transforms (against time) of all pixels in
    imgList.
    imgList is a list of tuples (datetime,image).
    Creates a 2 dimensional array, where one dimension is the pixel values in
    the image, and the other is time, then calculates the fourier transform.
    To give the frequency contributions of the values in each pixel.
    """
    (width,height) = cv.GetSize(imgList[0][1])
    nPixels = width * height
    print "Image Size = (%d x %d) - %d pixels.  Number of Images = %d" \
        %  (width,height,nPixels,len(imgList))

    # Create a matrix with pixel values in the y direction, and time (frame no)
    # in the x direction.   This means we can do an FFT on each row to get
    # frequency components of each pixel.
    dataMat = cv.CreateMat(len(imgList),nPixels,cv.CV_8UC1)
    for frameNo in range(len(imgList)):
        for y in range(height-1):
            for x in range(width-1):
                pixelNo = y*width+x
                pixelVal = imgList[frameNo][1][y,x]
                #print "frameNo=%d, x=%d, y=%d, pixelNo = %d, pixelVal = %d" % \
                #    (frameNo,x,y,pixelNo, pixelVal)
                dataMat[pixelNo,frameNo] = pixelVal
    
    print dataMat
    cv.ShowImage(window3,dataMat)

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
            # Preprocess, then add the new image to the list, along with the 
            # time it was recorded.
            imgList.append(
                (lastTime,
                 preProcessImage(origImg)
                 ))
            print "Number of Frames = %d" % (len(imgList))
            # Drop the oldest image off the list if we have enough in the list.
            if (len(imgList)>IMG_STACK_LEN):
                imgList.pop(0)  # Remove first item
                
            # Note - there is something odd about this time calculation
            # it does not seem to be consistent with the timestamps on the
            # images.
            timeDiff = (datetime.datetime.now() - lastTime).total_seconds() 
            fps = 1./timeDiff
            print "timeDiff=%f, fps=%f fps" % (timeDiff,fps)

            cv.ShowImage(window1,origImg)
            cv.ShowImage(window2,imgList[0][1])
            cv.WaitKey(1) # This is very important or ShowImage doesn't work!!
           
            if (len(imgList) == IMG_STACK_LEN):
                # imgList[] is now a list of tuples (time,image) containing the
                # reduced size images -
                spectra = getSpectra(imgList)

            # Now get a new frame ready to start the loop again
            origImg = cv.QueryFrame(camera)
            lastTime = datetime.datetime.now()
        print "no more images..."
    else:
        print "Error - failed to connect to camera"
# End of main()

if __name__ == "__main__":
    main()
