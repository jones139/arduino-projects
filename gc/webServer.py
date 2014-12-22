#!/usr/bin/python
#
#############################################################################
#
# Copyright Graham Jones, December 2013 
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
#    GC is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with GC.  If not, see <http://www.gnu.org/licenses/>.
##############################################################################
#
# This uses the bottle framework to make a simple web server
#
import time
import json
import bottle
#from bottle import route
from threading import Thread
import httplib2                     # Needed to communicate with camera


# This trick is taken from http://stackoverflow.com/questions/8725605/
#               bottle-framework-and-oop-using-method-instead-of-function

class WebServer():
    def __init__(self,ardCtrl):
        """ Initialise the web server as a separate thread.
        """
        self._ardCtrl = ardCtrl  # arduino Controller class instance that we will talk to.
        server = Thread(target = bottle.run, 
                        kwargs={'server':'cherrypy',
                                'host':'0.0.0.0',
                                'port':8080})
        server.setDaemon(True)
        server.start()

    def index(self):
        bottle.redirect("/static/index.html")
        #return "ok"

    def getChartImg(self):
        fname = self._chartImgFname
        return bottle.static_file(fname,root='/')

    def setSetPoint(self):
        setpoint=int(bottle.request.query.setpoint)
        print "setSetPoint(%d)" % setpoint
        self._ardCtrl.setSetPoint(setpoint)
        bottle.redirect("/")
        return "ok"

    def setKp(self):
        gain=int(bottle.request.query.setpoint)
        print "setKp(%d)" % gain
        self._ardCtrl.setKp(gain)
        bottle.redirect("/")

    def start(self):
        self._ardCtrl.start()
        bottle.redirect("/")
        return "ok"

    def stop(self):
        self._ardCtrl.stop()
        bottle.redirect("/")

        

    def staticFiles(self,filepath):
        """ Used to serve the static files from the /static path"""
        print filepath
        return self.serveStatic(filepath,True)

    def serveStatic(self,fname, static=False):
        if static:
            rootPath = './www'
        else:
            rootPath = './'

        rootPath = "./www"
        return bottle.static_file(fname,root=rootPath)


def setRoutes(app):
    """Initialise the web server routing - must be called with app set to be
    an instance of benWebServer.
    """
    bottle.route("/")(app.index)
    bottle.route("/static/<filepath:path>")(app.staticFiles)
    bottle.route("/chartImg")(app.getChartImg)
    bottle.route("/setSetPoint")(app.setSetPoint)
    bottle.route("/start")(app.start)    
    bottle.route("/stop")(app.stop)   
    bottle.route("/setKp")(app.setKp)
