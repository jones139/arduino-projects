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

import android.content.Context;
import android.content.res.AssetManager;
import android.content.res.AssetFileDescriptor;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.util.Log;
import android.widget.Toast;
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
     * Class describing the seizure detector web server - appears on port
     * 8080.
     */
    public class NanoHttpdWebServer extends NanoHTTPD {
	private String TAG = "WebServer";
	private GreenHouseData mGd;
	private Context mContext;
        public NanoHttpdWebServer(Context context, GreenHouseData gd, int port)
        {
	    // Set the port to listen on the specified port
            super(port);
	    mGd = gd;
	    mContext = context;
        }

        @Override
        public Response serve(String uri, Method method, 
                              Map<String, String> header,
                              Map<String, String> parameters,
                              Map<String, String> files) {
	    Log.v(TAG,"NanoHttpdWebServer.serve() - uri="+uri+" Method="+method.toString());
	    String answer = "Error - you should not see this message! - Something wrong in WebServer.serve()";

	    Iterator it = parameters.keySet().iterator();
	    while (it.hasNext()) {
		Object key = it.next();
		Object value = parameters.get(key);
		//Log.v(TAG,"Request parameters - key="+key+" value="+value);
	    }

	    if (uri.equals("/")) uri = "/index.html";
	    switch(uri) {
	    case "/data":
		//Log.v(TAG,"WebServer.serve() - Returning data");
		try {
		    answer = mGd.toString();
		} catch (Exception ex) {
		    Log.v(TAG,"Error Creating Data Object - "+ex.toString());
		    answer = "Error Creating Data Object";
		}
		break;

	    case "/settings":
		//Log.v(TAG,"WebServer.serve() - Returning settings");
		try {
		    JSONObject jsonObj = new JSONObject();
		    jsonObj.put("SettingsMessage","Settings Message Value");
		    answer = jsonObj.toString();
		} catch (Exception ex) {
		    Log.v(TAG,"Error Creating Data Object - "+ex.toString());
		    answer = "Error Creating Data Object";
		}
		break;

	    default:
		if (uri.startsWith("/index.html") ||
		    uri.startsWith("/favicon.ico") ||
		    uri.startsWith("/js/") ||
		    uri.startsWith("/css/") ||
		    uri.startsWith("/img/")) {
		    //Log.v(TAG,"Serving File");
		    return serveFile(uri);
		} else {
		    Log.v(TAG,"WebServer.serve() - Unknown uri -"+
			  uri);
		    answer = "Unknown URI: ";
		}
	    }
            return new NanoHTTPD.Response(answer);
        }
    

	
	/**
	 * Return a file from the apps /assets folder
	 */
	NanoHTTPD.Response serveFile(String uri) {
	    NanoHTTPD.Response res;
	    InputStream ip = null;
	    try {
		if (ip!=null) ip.close();
		String assetPath = "www";
		String fname = assetPath+uri;
		//Log.v(TAG,"serveFile - uri="+uri+", fname="+fname);
		AssetManager assetManager = mContext.getResources().getAssets();
		ip = assetManager.open(fname);
		String mimeStr = "text/html";
		res = new NanoHTTPD.Response(NanoHTTPD.Response.Status.OK,
					     mimeStr,ip);
		res.addHeader("Content-Length", "" + ip.available());
	    } catch (IOException ex) {
		Log.v(TAG,"serveFile(): Error Opening File - "+ex.toString());
		res = new NanoHTTPD.Response("serveFile(): Error Opening file "+uri);
	    } 
	    return(res);
	}
    }
