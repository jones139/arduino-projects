#/usr/bin/python

# Based on example found at:
# http://rwsarduino.blogspot.co.uk/2014/12/python-plots-from-serial-input.html
#
# Needs some work because sometimes null bytes mess up the 'split' function
# and it crashes.
#

import matplotlib
matplotlib.use('agg')
import matplotlib.pyplot as plt
import time
import serial

MAXLEN = 300

print "plotdaemon.py"
print "Opening Serial Connection"
ser = serial.Serial('/dev/ttyUSB0',9600,timeout=1)
print "Waiting for serial comms to settle..."
time.sleep(2)
print "Reading first line of data, and ignoring it."
line = ser.readline() # throw away any part lines
print "line=%s" % line
print "Sending start command"
ser.write(b'start')
print "ignoring a few lines for start command to register..."
for n in range(0,5):
  line = ser.readline() # throw away any part lines
  print "line=%s" % line

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
  mylist = line.split('\r')[0].split(",") # parse it into CSV tokens
  if (mylist[0]=='data'):
    print mylist
    now = float(mylist[1])/1000 # time now in seconds
    t.append(float(mylist[1])/1000) # from first element as milliseconds
    d1.append(float(mylist[2])) # six data elements added to lists
    d2.append(float(mylist[3]))
    d3.append(float(mylist[4]))
    d4.append(float(mylist[5]))
    d5.append(float(mylist[6]))

    #Trim the lists to avoid using up all the memory!
    if (len(t)>MAXLEN):
      #print len(t),t
      del t[0:(len(t)-MAXLEN)]
      #print len(t),t
      del d1[0:(len(d1)-MAXLEN)]
      del d2[0:(len(d2)-MAXLEN)]
      del d3[0:(len(d3)-MAXLEN)]
      del d4[0:(len(d4)-MAXLEN)]
      del d5[0:(len(d5)-MAXLEN)]
      
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
    #plt.draw()
      plt.savefig('out.png')
  else:
    print "Message %s" % line.split('\n')[0]
