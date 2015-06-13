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

import android.text.format.Time;
import android.util.Log;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;

import java.io.IOException;
import java.io.InputStream;
import java.util.Set;
import java.util.UUID;

import org.json.JSONObject;
import org.json.JSONArray;

/**
 * GreenHouseController is the class that talks to the Arduino microcontroller
 * via bluetooth to obtain greenhouse data and control equipment.
 */
public class GreenHouseController {
    private final static String TAG = "GreenHouseController";
    GreenHouseData mGd;
    private String mDeviceId;
    private UUID mDeviceUUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"); // Standard SPP UUID
    private BluetoothAdapter mBTAdapter;
    private BluetoothDevice mDevice;
    private BluetoothSocket mBTSocket;
    private boolean mIsBluetoothConnected = false;
    private boolean mConnectSuccessful = false;
    private ReadInput mReadThread = null;



    public GreenHouseController(GreenHouseData gd, String deviceId) {
	Log.v(TAG,"GreenHouseController()");
	mGd = gd;
	mDeviceId = deviceId;
    }

    /**
     * Connect to the bluetooth device and start the message reading
     * thread.
     * Received messages are processed by the handleMessage() function.
     * Messages are sent by the sendMessage() function.
     *
     */
    public void start() {
	Log.v(TAG,"Searching for device named: "+mDeviceId);
	mBTAdapter = BluetoothAdapter.getDefaultAdapter();
	Set<BluetoothDevice> paired = mBTAdapter.getBondedDevices();
	if (paired.size() > 0) {
	    for (BluetoothDevice d : paired) {
		Log.v(TAG,"Paired Device: "+d.getName());
		if (d.getName().equals(mDeviceId)) {
		    Log.v(TAG,"Found target device: "+d.getName());
		    mDevice = d;
		}
	    }
	}
	Log.v(TAG,"Connecting to Bluetooth Device");
	try {
	    if (mBTSocket == null || !mIsBluetoothConnected) {
		mBTSocket = mDevice.createRfcommSocketToServiceRecord(mDeviceUUID);
		BluetoothAdapter.getDefaultAdapter().cancelDiscovery();
		mBTSocket.connect();
		mConnectSuccessful = true;
		mReadThread = new ReadInput(); // Kick off input reader
	    }
	} catch (IOException e) {
	    // Unable to connect to device
	    e.printStackTrace();
	    mConnectSuccessful = false;
	}
	
    }

    /**
     * Stop the bluetooth receiver thread.
     */
    public void stop() {
	if (mReadThread != null) {
	    mReadThread.stop();
	}

	try {
	    mBTSocket.close();
	} catch (IOException e) {
	    e.printStackTrace();
	}
    }


    public void sendMessage(String msg) {
	try {
	    mBTSocket.getOutputStream().write(msg.getBytes());
	    mBTSocket.getOutputStream().write('\n');
	} catch (IOException e) {
	    // TODO Auto-generated catch block
	    e.printStackTrace();
	}
    }


    private void handleMessage(String msg) {
	Log.v(TAG,"HandleMessage("+msg+")");
	String[] msgLines = msg.split("\n");
	for (String msgLine : msgLines) {
	    Log.v(TAG,"msgLine="+msgLine);
	    if (msgLine.charAt(0)=='{') {
		Log.v(TAG,"HandleMessage - parsing JSON object"+msgLine);
		try {
		    JSONObject jo = new JSONObject(msgLine);
		    Log.v(TAG,"fromJSON(): jo = "+jo.toString());
		    dataTime.setToNow();
		    Log.v(TAG,"handleMessage(): dataTime = "+dataTime.toString());
		    pulsesPerLitre = jo.optInt("ppl");
		    baseWaterRate = jo.optInt("bwr");
		    baseWaterTemp = jo.optInt("bwt");
		    waterTempCoef = jo.optInt("wrc");
		    nWatering = jo.optInt("nwa");
		    pulseWarnThresh = jo.optInt("pwt");
		    decayFac = jo.optInt("dec");
		    samplePeriod = jo.optInt("spr");
		    serRes = jo.optInt("srv");
		    decayFac = jo.optInt("dec");
		} catch (Exception e) {
		    Log.v(TAG,"fromJSON() - error parsing result");
		}
		
		
	    } else {
		Log.v(TAG,"HandleMessage - not a JSON object - ignoring");
	    }
	}

    }

    private class ReadInput implements Runnable {
	private boolean bStop = false;
	private Thread t;

	public ReadInput() {
	    t = new Thread(this, "Input Thread");
	    t.start();
	}

	public boolean isRunning() {
	    return t.isAlive();
	}

	@Override
	public void run() {
	    InputStream inputStream;
	    try {
		inputStream = mBTSocket.getInputStream();
		while (!bStop) {
		    byte[] buffer = new byte[256];
		    if (inputStream.available() > 0) {
			inputStream.read(buffer);
			int i = 0;
			/*
			 * This is needed because new String(buffer) is taking the entire buffer i.e. 256 chars on Android 2.3.4 http://stackoverflow.com/a/8843462/1287554
			 */
			for (i = 0; i < buffer.length && buffer[i] != 0; i++) {
			}
			final String strInput = new String(buffer, 0, i);
			Log.v(TAG,"Received Message: "+strInput);
			handleMessage(strInput);
		    }
		    Thread.sleep(500);
		}
	    } catch (IOException e) {
		// TODO Auto-generated catch block
		e.printStackTrace();
	    } catch (InterruptedException e) {
		// TODO Auto-generated catch block
		e.printStackTrace();
	    }
	    
	}
	
	public void stop() {
	    bStop = true;
	}
	
    }


    public void waterOn() {
	Log.v(TAG,"waterOn()");
	sendMessage("wateron");
	sendMessage("{water='on'}");
    }

    public void waterOff() {
	Log.v(TAG,"waterOff()");
	sendMessage("wateroff");
	sendMessage("{water='off'}");
    }


    public void getSettings() {
	sendMessage("set");
    }

    public void getData() {
	sendMessage("data");
    }
}
