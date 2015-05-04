/*
  Greenhouse_Controller - monitor and control greenhouse using android phone
  and bluetooth connected arduino microcontroller.

  Copyright Graham Jones, 2015.

  This file is part of Greenhouse_Controller.

  Greenhouse_Controller is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  Greenhouse_Controller is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with Greenhouse_Controller.  If not, see <http://www.gnu.org/licenses/>.

*/


package uk.org.openseizuredetector.greenhouse;

import android.util.Log;
import com.aronbordin.BluetoothArduino;

/**
 * GreenHouseController is the class that talks to the Arduino microcontroller
 * via bluetooth to obtain greenhouse data and control equipment.
 */
public class GreenHouseController {
    private final static String TAG = "GreenHouseController";
    GreenHouseData mGd;
    BluetoothArduino mBlue;
    public GreenHouseController(GreenHouseData gd) {
	Log.v(TAG,"GreenHouseController()");
	mGd = gd;
	Log.v(TAG,"Connecting to Bluetooth Device");
	mBlue = BluetoothArduino.getInstance("GreenHouseController");
	mBlue.Connect();
    }

    public void waterOn() {
	Log.v(TAG,"waterOn()");
	mBlue.SendMessage("water=1");
    }

    public void waterOff() {
	Log.v(TAG,"waterOff()");
	mBlue.SendMessage("water=0");
    }



}
