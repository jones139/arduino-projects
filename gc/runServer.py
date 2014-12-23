#!/usr/bin/python
#
#############################################################################
#
# Copyright Graham Jones, 2013-2014
#
#############################################################################
#
#   This file is part of GC.
#
#    GC is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    Foobar is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with GC.  If not, see <http://www.gnu.org/licenses/>.
##############################################################################
#
"""[application description here]"""
 
__appname__ = "runServer"
__author__  = "Graham Jones"
__version__ = "0.1"
__license__ = "GNU GPL 3.0 or later"
import os, sys, time,serial
from config_utils import ConfigUtil
import webServer
import matplotlib
matplotlib.use('agg')
import matplotlib.pyplot as plt

class runServer(object):
    configFname = "config.ini"
    configSection = "server"
    MAXLEN = 300

    def __init__(self,port = None):
        print "runServer.__init__()"
        print os.path.realpath(__file__)
        configPath = "%s/%s" % (os.path.dirname(os.path.realpath(__file__)),
                                self.configFname)
        print configPath
        self.cfg = ConfigUtil(configPath,self.configSection)

        self.debug = self.cfg.getConfigBool("debug")
        if (self.debug): print "Debug Mode"

        self._wkdir = self.cfg.getConfigStr("working_directory")
        if (self.debug): print "working_directory=%s\n" % self._wkdir
        self._tmpdir = self.cfg.getConfigStr("tmpdir")
        if (self.debug): print "tmpdir=%s\n" % self._tmpdir


        print "Opening Serial Connection"
        if (port == None):
            port = '/dev/ttyUSB0'
        self.ser = serial.Serial(port,9600,timeout=1)
        print "Waiting for serial comms to settle..."
        time.sleep(2)
        print "Reading first line of data, and ignoring it."
        line = self.ser.readline() # throw away any part lines
        print "line=%s" % line

        self.start()
        print "waiting for some data..."
        while(self.ser.inWaiting() < 10): 
            print "waiting - inWaiting()=%d" % self.ser.inWaiting()
            time.sleep(1)
        print "wait over!"
        print "ignoring a few lines for start command to register..."
        for n in range(0,5):
            line = self.ser.readline() # throw away any part lines
            print "line=%s" % line
    
        #Initialise data lists.
        self.t=[] # initialize the data lists
        self.d1=[]
        self.d2=[]
        self.d3=[]
        self.d4=[]
        self.d5=[]
        self.d6=[]


        # Start web server
        self._ws = webServer.WebServer(self)
        webServer.setRoutes(self._ws)

        # Run our main loop.
        self.run()
    
    def run(self):
        """Run the main loop."""
        while(True):
            line = self.ser.readline() # read a line of text
            mylist = line.split('\r')[0].split(",") # parse it into CSV tokens
            if (mylist[0]=='data'):
                print mylist
                now = float(mylist[1])/1000 # time now in seconds
                self.t.append(float(mylist[1])/1000) # from first element as milliseconds
                self.d1.append(float(mylist[2])) # six data elements added to lists
                self.d2.append(float(mylist[3]))
                self.d3.append(float(mylist[4]))
                self.d4.append(float(mylist[5]))
                self.d5.append(float(mylist[6]))

                #Trim the lists to avoid using up all the memory!
                lenLists = len(self.t)
                if (lenLists>self.MAXLEN):
                    del self.t[0:(lenLists-self.MAXLEN)]
                    del self.d1[0:(lenLists-self.MAXLEN)]
                    del self.d2[0:(lenLists-self.MAXLEN)]
                    del self.d3[0:(lenLists-self.MAXLEN)]
                    del self.d4[0:(lenLists-self.MAXLEN)]
                    del self.d5[0:(lenLists-self.MAXLEN)]

                if(self.ser.inWaiting() < 100): # redraw only if you are caught up
                    plt.clf() # clear the figure
                    plt.plot(self.t,self.d1) # plot a line for each set of data
                    plt.plot(self.t,self.d2)
                    plt.plot(self.t,self.d3)
                    plt.plot(self.t,self.d4)
                    plt.plot(self.t,self.d5)
                    #plt.axis([now-300,now,min(d1)-50,max(d1)+50])
                    plt.axis([now-300,now,0,1000])
                    plt.xlabel("Time Since Boot [s]")
                    plt.grid(b=True, which='both', color='0.65',linestyle='-')
                    #plt.draw()
                    plt.savefig('www/out.png')
            elif (mylist[0]=='Set'):
                    self.settingsStr = line  # used by 'settings' function.
                    self.haveSettings = True
            else:
                if (len(line.split('\n')[0])>0):
                    print "Message %s" % line.split('\n')[0]

    def setSetPoint(self,setpoint):
        print "setSetPoint(%d)" % setpoint
        self.ser.write("setpoint=%d" % setpoint)

    def setKp(self,gain):
        print "setKp(%d)" % gain
        self.ser.write("kp=%d" % gain)

    def setKi(self,gain):
        print "setKi(%d)" % gain
        self.ser.write("ki=%d" % gain)

    def setKd(self,gain):
        print "setKd(%d)" % gain
        self.ser.write("kd=%d" % gain)

    def start(self):
        print "Sending start command"
        self.ser.write("start")
        print "returning to main loop...."

    def stop(self):
        print "sending stop command"
        self.ser.write("stop")

    def settings(self):
        self.haveSettings = False    # Flag - set by main loop if settings received.
        print "Requesting settings"
        self.ser.write("settings")

        print "Waiting for settings to be returned..."
        # wait for main loop to set haveSettings.
        while (not self.haveSettings):  
            pass
        print "found settings: %s" % self.settingsStr
        return self.settingsStr
    
if __name__=="__main__":
    # Boilerplate code from https://gist.github.com/ssokolow/151572
    from optparse import OptionParser
    parser = OptionParser(version="%%prog v%s" % __version__,
            usage="%prog [options] <argument> ...",
            description=__doc__.replace('\r\n', '\n').split('\n--snip--\n')[0])
    parser.add_option('-p', '--port', dest="port",
        help="Specify port to connect to arduino (Default /dev/ttyUSB0).")
 
    opts, args  = parser.parse_args()
 
    print opts
    print args
    
    runServer(port=opts.port)
