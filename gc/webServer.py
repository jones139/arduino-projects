#!/usr/bin/python
#
#############################################################################
#
# Copyright Graham Jones, December 2014 
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
        return "ok"

    def setKp(self):
        kp=int(bottle.request.query.kp)
        print "setKp(%d)" % kp
        self._ardCtrl.setKp(kp)
        return "ok"


    def setKi(self):
        ki=int(bottle.request.query.ki)
        print "setKi(%d)" % ki
        self._ardCtrl.setKi(ki)
        return "ok"


    def setKd(self):
        kd=int(bottle.request.query.kd)
        print "setKd(%d)" % kd
        self._ardCtrl.setKd(kd)
        return "ok"

    def start(self):
        self._ardCtrl.start()
        return "ok"

    def stop(self):
        self._ardCtrl.stop()
        return "ok"

    def pumpstart(self):
        self._ardCtrl.pumpstart()
        return "ok"

    def pumpstop(self):
        self._ardCtrl.pumpstop()
        return "ok"

    def settings(self):
        setStr = self._ardCtrl.settings()
        print setStr
        return setStr

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
    bottle.route("/setKp")(app.setKp)
    bottle.route("/setKi")(app.setKi)
    bottle.route("/setKd")(app.setKd)
    bottle.route("/start")(app.start)    
    bottle.route("/stop")(app.stop)   
    bottle.route("/settings")(app.settings)
    bottle.route("/pumpstart")(app.pumpstart)    
    bottle.route("/pumpstop")(app.pumpstop)   
