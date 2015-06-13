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
  along with pebble_sd.  If not, see <http://www.gnu.org/licenses/>.

*/


package uk.org.openseizuredetector.greenhouse;

import java.util.Map;
import fi.iki.elonen.NanoHTTPD;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.ActivityManager.RunningServiceInfo;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetManager;
import android.content.res.AssetFileDescriptor;
import android.content.SharedPreferences;
import android.media.AudioManager;
import android.media.ToneGenerator;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Binder;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.Process;
import android.preference.PreferenceManager;
import android.telephony.SmsManager;
import android.telephony.SmsMessage;
import android.util.Log;
import android.widget.Toast;
import java.util.Timer;
import java.util.TimerTask;
import java.io.*;
import java.util.*;
import java.util.UUID;
import java.util.StringTokenizer;
import java.net.URL;
import android.net.Uri;
import java.nio.ByteBuffer;
import java.nio.ShortBuffer;
import java.nio.IntBuffer;
import java.nio.ByteOrder;
import android.text.format.Time;
import org.json.JSONObject;
import org.json.JSONArray;


/**
 * Based on example at:
 * http://stackoverflow.com/questions/14309256/using-nanohttpd-in-android
 * and 
 * http://developer.android.com/guide/components/services.html#ExtendingService
 */
public class GreenHouseServer extends Service
{
    // Notification ID
    private int NOTIFICATION_ID = 1;

    private NotificationManager mNM;

    private NanoHttpdWebServer webServer = null;
    private final static String TAG = "GreenHouseServer";
    private Looper mServiceLooper;
    public Time mPebbleStatusTime;
    private boolean mPebbleAppRunningCheck = false;
    private Timer statusTimer = null;
    private Timer settingsTimer = null;
    private Timer dataLogTimer = null;
    private HandlerThread thread;
    private WakeLock mWakeLock = null;
    public GreenHouseData mGd;
    public GreenHouseController mGc;
    private File mOutFile;

    private final IBinder mBinder = new GreenHouseBinder();

    /**
     * class to handle binding the MainApp activity to this service
     * so it can access sdData.
     */
    public class GreenHouseBinder extends Binder {
	GreenHouseServer getService() {
	    return GreenHouseServer.this;
	}
    }

    /**
     * Constructor for GreenHouseServer class - does not do much!
     */
    public GreenHouseServer() {
	super();
	mGd = new GreenHouseData();
	Log.v(TAG,"GreenHouseServer Created");
    }


    @Override
    public IBinder onBind(Intent intent) {
	Log.v(TAG,"sdServer.onBind()");
	return mBinder;
    }



    /**
     * onCreate() - called when services is created.  
     * Prepares a wake lock so that the web server continues to listen for
     * connection requests - lock is applied in the onStartCommand() function.
     */
    @Override
    public void onCreate() {
	Log.v(TAG,"onCreate()");
	PowerManager powerManager = 
	    (PowerManager) getSystemService(POWER_SERVICE);
	mWakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,
        "MyWakelockTag");
    }

    /**
     * onStartCommand - start the web server
     */
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
	Log.v(TAG,"onStartCommand() - GreenHouseServer service starting");
	
	// Update preferences.
	Log.v(TAG,"onStartCommand() - calling updatePrefs()");
	updatePrefs();
	
	// Display a notification icon in the status bar of the phone to
	// show the service is running.
	Log.v(TAG,"showing Notification");
	showNotification();

	// Start the web server
	startWebServer();

	// Start Greenhouse Controller
	startGreenHouseController();

	// Apply the wake-lock to prevent CPU sleeping (very battery intensive!)
	if (mWakeLock!=null) {
	    mWakeLock.acquire();
	    Log.v(TAG,"Applied Wake Lock to prevent device sleeping");
	} else {
	    Log.d(TAG,"mmm...mWakeLock is null, so not aquiring lock.  This shouldn't happen!");
	}
	return START_STICKY;
    }

    @Override
    public void onDestroy() {
	Log.v(TAG,"onDestroy(): GreenHouseServer Service stopping");

	mGc.stop();
	// release the wake lock to allow CPU to sleep and reduce
	// battery drain.
	if (mWakeLock!=null) {
	    mWakeLock.release();
	    Log.v(TAG,"Released Wake Lock to allow device to sleep.");
	} else {
	    Log.d(TAG,"mmm...mWakeLock is null, so not releasing lock.  This shouldn't happen!");
	}

	try {
	    // Cancel the notification.
	    Log.v(TAG,"onDestroy(): cancelling notification");
	    mNM.cancel(NOTIFICATION_ID);
	    // Stop web server
	    Log.v(TAG,"onDestroy(): stopping web server");
	    stopWebServer();
	    // stop this service.
	    Log.v(TAG,"onDestroy(): calling stopSelf()");
	    stopSelf();

	} catch(Exception e) {
	    Log.v(TAG,"Error in onDestroy() - "+e.toString());
	}
    }



    /**
     * Show a notification while this service is running.
     */
    private void showNotification() {
	Log.v(TAG,"showNotification()");
        CharSequence text = "GreenHouseServer Running";
        Notification notification = 
	   new Notification(R.drawable.icon_24x24, text,
			     System.currentTimeMillis());
	PendingIntent contentIntent = PendingIntent.getActivity(this, 0,
                new Intent(this, MainActivity.class), 0);
        notification.setLatestEventInfo(this, "GreenHouseServer",
                      text, contentIntent);
	notification.flags |= Notification.FLAG_NO_CLEAR;
        mNM = (NotificationManager)getSystemService(Context.NOTIFICATION_SERVICE);
        mNM.notify(NOTIFICATION_ID, notification);
    }


    /**
     * Start the greenhouse controller (communications with arduino 
     * microcontroller via bluetooth.
     */
    private void startGreenHouseController() {
	Log.v(TAG,"startGreenHouseController()");
	mGc = new GreenHouseController(mGd,"HC-06");
	mGc.start();
	mGc.waterOff();
    }

    /**
     * Stop the greenhouse controller (communications with arduino 
     * microcontroller via bluetooth.
     */
    private void stopGreenHouseController() {
	Log.v(TAG,"startGreenHouseController()");
	mGc.stop();
    }


    /**
     * Start the web server (on port 8080)
     */
    protected void startWebServer() {
	Log.v(TAG,"startWebServer()");
	if (webServer == null) {
	    // web server listens on port 8080
	    webServer = new NanoHttpdWebServer(getApplicationContext(),
					       mGd,8080);
	    try {
		webServer.start();
	    } catch(IOException ioe) {
		Log.w(TAG, "startWebServer(): Error: "+ioe.toString());
	    }
	    Log.w(TAG, "startWebServer(): Web server initialized.");
	} else {
	    Log.v(TAG, "startWebServer(): server already running???");
	}
    }

    /**
     * Stop the web server
     */
    protected void stopWebServer() {
	Log.v(TAG,"stopWebServer()");
	if (webServer!=null) {
	    webServer.stop();
	    if (webServer.isAlive()) {
		Log.v(TAG,"stopWebServer() - server still alive???");
	    } else {
		Log.v(TAG,"stopWebServer() - server died ok");
	    }
	    webServer = null;
	}
    }

    /**
     * updatePrefs() - update basic settings from the SharedPreferences
     * - defined in res/xml/prefs.xml
     */
    public void updatePrefs() {
	Log.v(TAG,"updatePrefs()");
	SharedPreferences SP = PreferenceManager
	    .getDefaultSharedPreferences(getBaseContext());
	try {
	    //mAudibleFaultWarning = SP.getBoolean("AudibleFaultWarning",true);
	    //Log.v(TAG,"updatePrefs() - mAuidbleFaultWarning = "+mAudibleFaultWarning);
	} catch (Exception ex) {
	    Log.v(TAG,"updatePrefs() - Problem parsing preferences!");
	    Toast toast = Toast.makeText(getApplicationContext(),"Problem Parsing Preferences - Something won't work - Please go back to Settings and correct it!",Toast.LENGTH_SHORT);
	    toast.show();
	}
    }



}
