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

import android.os.Parcelable;
import android.os.Parcel;
import android.text.format.Time;
import android.util.Log;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Locale;
import org.json.JSONObject;
import org.json.JSONArray;

/* based on http://stackoverflow.com/questions/2139134/how-to-send-an-object-from-one-android-activity-to-another-using-intents */

public class GreenHouseData implements Parcelable {
    private final static String TAG = "GreenHouseData";
    /* Analysis settings */
    public boolean haveSettings = false;   // flag to say if we have received settings or not.
    // Settings
    private int pulsesPerLitre = 0;
    private int baseWaterRate = 0;
    private int nWatering = 1;
    private int baseWaterTemp = 18;
    private int waterTempCoef = 1;
    private int decayFac = 1;
    private int samplePeriod = 3600;
    private int serRes=100000;
    private int pulseWarnThresh = 20;

    // variable data
    private Time dataTime;
    private int timeMs = 0;
    private float curTemp = 0;
    private float avTemp = 0;
    private int soilm = 0;
    private int waterRate = 0;
    private int curVol = 0;
    private int pumpStatus = 0;
    private int warnStatus = 0;


    public GreenHouseData() {
	dataTime = new Time(Time.getCurrentTimezone());
    }

    /*
     * Intialise this GreenHouseData object from a JSON String
     */
    public boolean fromJSON(String jsonStr) {
	Log.v(TAG,"fromJSON() - parsing jsonString - "+jsonStr);
	try {
	    JSONObject jo = new JSONObject(jsonStr);
	    Log.v(TAG,"fromJSON(): jo = "+jo.toString());
	    
	    return true;
	} catch (Exception e) {
	    Log.v(TAG,"fromJSON() - error parsing result");
	    return false;
	}


    }


    public String toString() {
	return toDataString();
    }

    public String toDataString() {
	String retval;
	retval = "GreenHouseData.toDataString() Output";
		try {
		    JSONObject jsonObj = new JSONObject();
		    if (dataTime != null) {
			jsonObj.put("dataTime",dataTime.format("%d-%m-%Y %H:%M:%S"));
			jsonObj.put("dataTimeStr",dataTime.format("%Y%m%dT%H%M%S"));
		    } else {
			jsonObj.put("dataTimeStr","00000000T000000");
			jsonObj.put("dataTime","00-00-00 00:00:00");
		    }
		    Log.v(TAG,"GreenHouseData.dataTime = "+dataTime);
		    jsonObj.put("maxVal",maxVal);
		    jsonObj.put("maxFreq",maxFreq);
		    jsonObj.put("specPower",specPower);
		    jsonObj.put("roiPower",roiPower);
		    jsonObj.put("batteryPc",batteryPc);
		    jsonObj.put("pebbleConnected",pebbleConnected);
		    jsonObj.put("pebbleAppRunning",pebbleAppRunning);
		    jsonObj.put("alarmState",alarmState);
		    jsonObj.put("alarmPhrase",alarmPhrase);
		    JSONArray arr = new JSONArray();
		    for (int i=0;i<simpleSpec.length;i++) {
			arr.put(simpleSpec[i]);
		    }

		    jsonObj.put("simpleSpec",arr);

		    retval = jsonObj.toString();
		} catch (Exception ex) {
		    Log.v(TAG,"Error Creating Data Object - "+ex.toString());
		    retval = "Error Creating Data Object - "+ex.toString();
		}

	return(retval);
    }

    public int describeContents() {
	return 0;
    }

    public void writeToParcel(Parcel outParcel, int flags) {
	//outParcel.writeInt(fMin);
	//outParcel.writeInt(fMax);
    }

    private GreenHouseData(Parcel in) {
	//fMin = in.readInt();
	//fMax = in.readInt();
    }

    public static final Parcelable.Creator<GreenHouseData> CREATOR = new Parcelable.Creator<GreenHouseData>() {
	public GreenHouseData createFromParcel(Parcel in) {
	    return new GreenHouseData(in);
	}
	public GreenHouseData[] newArray(int size) {
	    return new GreenHouseData[size];
	}
    };

}
