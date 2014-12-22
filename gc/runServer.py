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
import os, sys, time
from config_utils import ConfigUtil
import webServer

class runServer(object):
    configFname = "config.ini"
    configSection = "server"

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


        self._ws = webServer.WebServer(self)
        webServer.setRoutes(self._ws)
        self.run()
    
    def run(self):
        """Run the main loop."""
        while(True):
            sys.stderr.write('.')
            time.sleep(1)
                        

    def setSetPoint(self,setpoint):
        print "setSetPoint(%d)" % setpoint
    
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
