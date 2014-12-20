#/usr/bin/python

import matplotlib.pyplot as plt
import time
import serial

plt.ion()
ser = serial.Serial('/dev/ttyUSB1',9600,timeout=1)
line = ser.readline() # throw away any part lines
while(ser.inWaiting() < 100): # make sure something is coming
  now = 0.0
t=[] # initialize the data lists
d1=[]
d2=[]
d3=[]
d4=[]
d5=[]
d6=[]
while (ser.isOpen()):
  line = ser.readline() # read a line of text
  print line
  mylist = line.split(",") # parse it into CSV tokens
  print mylist
  now = float(mylist[0])/1000 # time now in seconds
  t.append(float(mylist[0])/1000) # from first element as milliseconds
  d1.append(float(mylist[1])) # six data elements added to lists
  d2.append(float(mylist[2]))
  d3.append(float(mylist[3]))
  d4.append(float(mylist[4]))
  d5.append(float(mylist[5]))
  #d6.append(float(mylist[6]))
  if(ser.inWaiting() < 100): # redraw only if you are caught up
    plt.clf() # clear the figure
    plt.plot(t,d1) # plot a line for each set of data
    plt.plot(t,d2)
    plt.plot(t,d3)
    plt.plot(t,d4)
    plt.plot(t,d5)
    #plt.plot(t,d6)
    #plt.axis([now-300,now,min(d1)-50,max(d1)+50])
    plt.axis([now-300,now,0,1000])
    plt.xlabel("Time Since Boot [s]")
    plt.grid(b=True, which='both', color='0.65',linestyle='-')
    plt.draw()
