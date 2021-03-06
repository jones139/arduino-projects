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

import android.app.Activity;
import android.app.IntentService;
import android.app.ActivityManager;
import android.app.ActivityManager.RunningServiceInfo;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.PackageInfo;
import android.preference.Preference;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.Context;
import android.graphics.Color;
import android.media.AudioManager;
import android.media.ToneGenerator;
import android.net.Uri;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiInfo;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewConfiguration;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Button;
import java.lang.reflect.Field;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.Timer;
import java.util.TimerTask;
import org.apache.http.conn.util.InetAddressUtils;
import java.lang.CharSequence;
import android.util.AttributeSet;

import uk.org.openseizuredetector.greenhouse.GreenHouseServer;

public class MainActivity extends Activity
{
    static final String TAG = "MainActivity";
    private int okColour = Color.BLUE;
    private int warnColour = Color.MAGENTA;
    private int alarmColour = Color.RED;
    GreenHouseServer mGreenHouseServer;
    boolean mBound = false;
    private Menu mOptionsMenu;

    private Intent greenHouseServerIntent;

    final Handler serverStatusHandler = new Handler();
    Messenger messenger = new Messenger(new ResponseHandler());

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

	// Initialise the User Interface
        setContentView(R.layout.main);

	/* Force display of overflow menu - from stackoverflow
	 * "how to force use of..."
	 */
	try {
	    ViewConfiguration config = ViewConfiguration.get(this);
	    Field menuKeyField =
		ViewConfiguration.class.getDeclaredField("sHasPermanentMenuKey");
	    if (menuKeyField!=null) {
		menuKeyField.setAccessible(true);
		menuKeyField.setBoolean(config,false);
	    }
	} catch (Exception e) {
	    Log.v(TAG,"menubar fiddle exception: "+e.toString());
	}


	// start timer to refresh user interface every second.
	Timer uiTimer = new Timer();
	uiTimer.schedule(new TimerTask() {
		@Override
		public void run() {updateServerStatus();}
	    }, 0, 1000);	

    }

    /**
     * Create Action Bar
     */
    @Override
    public boolean onCreateOptionsMenu (Menu menu)
    {
	getMenuInflater().inflate(R.menu.main_activity_actions,menu);
	mOptionsMenu = menu;
	return true;
    }
    
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
	Log.v(TAG,"Option "+item.getItemId()+" selected");
	Log.v(TAG,"action_start_stop = "+R.id.action_start_stop);
	switch (item.getItemId()) {
	case R.id.action_start_stop:
	    // Respond to the start/stop server menu item.
	    Log.v(TAG,"action_sart_stop");
	    if (mBound) {
		Log.v(TAG,"Stopping Server");
		unbindFromServer();
		stopServer();
	    } else {
		Log.v(TAG,"Starting Server");
		startServer();
		// and bind to it so we can see its data
		bindToServer();
	    }
	    return true;
	case R.id.action_settings:
	    Log.v(TAG,"action_settings");
	    try {
		Intent prefsIntent = new Intent(
						MainActivity.this,
						PrefActivity.class);
		this.startActivity(prefsIntent);
	    } catch (Exception ex) {
		Log.v(TAG,"exception starting settings activity "+ex.toString());
	    }
	    return true;
	default:
	    return super.onOptionsItemSelected(item);
	}
    }

    @Override
    protected void onStart() {
	super.onStart();
	SharedPreferences SP = PreferenceManager
	    .getDefaultSharedPreferences(getBaseContext());
	boolean audibleAlarm = SP.getBoolean("AudibleAlarm",true);
	Log.v(TAG,"onStart - auidbleAlarm = "+audibleAlarm);

	TextView tv;
	tv = (TextView) findViewById(R.id.versionTv);
	String versionName = "unknown";
	// From http://stackoverflow.com/questions/4471025/
	//         how-can-you-get-the-manifest-version-number-
	//         from-the-apps-layout-xml-variable
	final PackageManager packageManager = getPackageManager();
	if (packageManager != null) {
	    try {
		PackageInfo packageInfo = packageManager.getPackageInfo(getPackageName(), 0);
		versionName = packageInfo.versionName;
	    } catch (PackageManager.NameNotFoundException e) {
		Log.v(TAG,"failed to find versionName");
		versionName = null;
	    }
	}
	tv.setText("GreenHouseServer Version "+versionName);

	if (!isServerRunning()) {
	    Log.v(TAG,"Server not Running - Starting Server");
	    startServer();
	} else {
	    Log.v(TAG,"Server Already Running OK");
	}
	// and bind to it so we can see its data
	bindToServer();

    }

    @Override
    protected void onStop() {
	super.onStop();
	unbindFromServer();
    }

  /** Defines callbacks for service binding, passed to bindService() */
    private ServiceConnection mConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName className,
                IBinder service) {
            // We've bound to LocalService, cast the IBinder and get LocalService instance
            GreenHouseServer.GreenHouseBinder binder = (GreenHouseServer.GreenHouseBinder) service;
            mGreenHouseServer = binder.getService();
            mBound = true;
	    if (mGreenHouseServer!=null) {
		Log.v(TAG,"onServiceConnected() - Asking server to update its settings");
		mGreenHouseServer.updatePrefs();
	    }
	    else {
		Log.v(TAG,"onServiceConnected() - mGreenHouseServer is null - this is wrong!");
	    }
        }

        @Override
        public void onServiceDisconnected(ComponentName arg0) {
	    Log.v(TAG,"onServiceDisonnected()");
            mBound = false;
        }
    };


    /**
     * bind to an already running server.
     */
    private void bindToServer() {
	Log.v(TAG,"bindToServer() - binding to SdServer");
	Intent intent = new Intent(this,GreenHouseServer.class);
	bindService(intent,mConnection, Context.BIND_AUTO_CREATE);
    }

    /** 
     * unbind from server
     */
    private void unbindFromServer() {
	// unbind this activity from the service if it is bound.
	if (mBound) {
	    Log.v(TAG,"unbindFromServer() - unbinding");
	    try {
		unbindService(mConnection);
		mBound = false;
	    } catch (Exception ex) {
		Log.e(TAG,"unbindFromServer() - error unbinding service - "+ex.toString());
	    }
	} else {
	    Log.v(TAG,"unbindFromServer() - not bound to server - ignoring");
	}
    }

    /**
     * Start the GreenHouseServer service
     */
    private void startServer() {
	// Start the server
	greenHouseServerIntent = new Intent(MainActivity.this,GreenHouseServer.class);
	greenHouseServerIntent.setData(Uri.parse("Start"));
	getApplicationContext().startService(greenHouseServerIntent);

	// Change the action bar icon to show the option to stop the service.
	if (mOptionsMenu!=null) {
	    Log.v(TAG,"Changing menu icons");
	    MenuItem menuItem = mOptionsMenu.findItem(R.id.action_start_stop);
	    menuItem.setIcon(R.drawable.stop_server);
	    menuItem.setTitle("Stop Server");
	} else {
	    Log.v(TAG,"mOptionsMenu is null - not changing icons!");
	}
    }

    /**
     * Stop the GreenHouseServer service
     */
    private void stopServer() {
	Log.v(TAG,"stopping Server...");

	// then send an Intent to stop the service.
	greenHouseServerIntent = new Intent(MainActivity.this,
					    GreenHouseServer.class);
	greenHouseServerIntent.setData(Uri.parse("Stop"));
	getApplicationContext().stopService(greenHouseServerIntent);

	// Change the action bar icon to show the option to start the service.
	if (mOptionsMenu!=null) {
	    Log.v(TAG,"Changing action bar icons");
	    mOptionsMenu.findItem(R.id.action_start_stop).setIcon(R.drawable.start_server);
	    mOptionsMenu.findItem(R.id.action_start_stop).setTitle("Start Server");
	} else {
	    Log.v(TAG,"mOptionsMenu is null, not changing icons!");
	}

    }

    /**
     * Based on http://stackoverflow.com/questions/7440473/android-how-to-check-if-the-intent-service-is-still-running-or-has-stopped-running
     */
    public boolean isServerRunning() {
	//Log.v(TAG,"isServerRunning()................");
	ActivityManager manager = 
	    (ActivityManager) getSystemService(ACTIVITY_SERVICE);
	for (RunningServiceInfo service : 
		 manager.getRunningServices(Integer.MAX_VALUE)) {
	    //Log.v(TAG,"Service: "+service.service.getClassName());
	    if ("uk.org.openseizuredetector.greenhouse.GreenHouseServer"
		.equals(service.service.getClassName())) {
		//Log.v(TAG,"Yes!");
            return true;
	    }
	}
	//Log.v(TAG,"No!");
	return false;
    }


    /** get the ip address of the phone.
     * Based on http://stackoverflow.com/questions/11015912/how-do-i-get-ip-address-in-ipv4-format
     */
    public String getLocalIpAddress() {
	try {
	    for (Enumeration<NetworkInterface> en = NetworkInterface
		     .getNetworkInterfaces(); en.hasMoreElements();) {
		NetworkInterface intf = en.nextElement();
		for (Enumeration<InetAddress> enumIpAddr = intf
			 .getInetAddresses(); enumIpAddr.hasMoreElements();) {
		    InetAddress inetAddress = enumIpAddr.nextElement();
		    //Log.v(TAG,"ip1--:" + inetAddress);
		    //Log.v(TAG,"ip2--:" + inetAddress.getHostAddress());
		
		    // for getting IPV4 format
		    if (!inetAddress.isLoopbackAddress() 
			&& InetAddressUtils.isIPv4Address(
				 inetAddress.getHostAddress())) {
		    
			String ip = inetAddress.getHostAddress().toString();
			//Log.v(TAG,"ip---::" + ip);
			return ip;
		    }
		}
	    }
	} catch (Exception ex) {
	    Log.e("IP Address", ex.toString());
	}
	return null;
    }



    /*
     * updateServerStatus - called by the uiTimer timer periodically.
     * requests the ui to be updated by calling serverStatusRunnable.
     */
    private void updateServerStatus() {
	serverStatusHandler.post(serverStatusRunnable);
    }
	
    /*
     * serverStatusRunnable - called by updateServerStatus - updates the
     * user interface to reflect the current status received from the server.
     */
    final Runnable serverStatusRunnable = new Runnable() {
	    public void run() {
		TextView tv;
		tv = (TextView) findViewById(R.id.textView1);
		if (isServerRunning()) {
		    tv.setText("Server Running OK");
		    tv.setBackgroundColor(okColour);
		    tv = (TextView)findViewById(R.id.textView2);
		    tv.setText("Access Server at http://"
				   +getLocalIpAddress()
				   +":8080");
		    tv.setBackgroundColor(okColour);
		} else {
		    tv.setText("*** Server Stopped ***");
		    tv.setBackgroundColor(alarmColour);
		}


		try {
		    if (mBound) {
			tv = (TextView) findViewById(R.id.alarmTv);
			if (mGreenHouseServer.mGd.alarmState==0) {
			    tv.setText(mGreenHouseServer.mGd.alarmPhrase);
			    tv.setBackgroundColor(okColour);
			}
		    }
		} catch (Exception e) {
		    Log.v(TAG,"ServerStatusRunnable: Exception - "+e.toString());
		}
	    }
	};
    

    @Override
    protected void onPause() {
	super.onPause();
    }
    @Override
    protected void onResume() {
	super.onResume();
    }

    class ResponseHandler extends Handler {
	@Override public void handleMessage(Message message) {
	    Log.v(TAG,"Message="+message.toString());
	}
    }

}
