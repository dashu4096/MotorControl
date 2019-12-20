/**
* @file MotorController.java
* @brief 
* @author Hansen.Z
* @version 1.0
* @date 2019-08-11
*/
package com.paydevice.demo.motorcontrol;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;
import android.util.Log;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

/**
* @brief Class of MotorController implement
*/
public class MotorController {
	private static final String TAG = "MotorController";
    private static final String ACTION_USB_PERMISSION = "com.paydevice.demo.motorcontrol.USB_PERMISSION";
	//vid,pid of controller
    private static final int VID = 0x1a86;
    private static final int PID = 0x5722;

	//init command after connected
	private byte [] cmd_init = { 0x1B,0x00 };
	//reset command cause controller reset
	private byte [] cmd_reset = { 0x1B,0x40 };
	//get unique id of controller
	private byte [] cmd_get_id = { 0x1B,0x43 };
	//trigger motror commands
	private byte [][] cmd_motor_list = {
		{0x1B,0x30},//motor 1 
		{0x1B,0x31},//motor 2
		{0x1B,0x32},//motor 3
		{0x1B,0x33},//motor 4
		{0x1B,0x34},//motor 5
		{0x1B,0x35},//motor 6
	};

	private final Context mContext;
    private UsbDevice mUsbDevice;
    private UsbManager mUsbManager;
    private UsbEndpoint mEpWrite;
    private UsbEndpoint mEpRead;
    private UsbDeviceConnection mUsbConnection;

	public MotorController(Context context) {
		mContext = context;
		mUsbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
	}

	/**
	* @brief probe controller and connect it
	*
	* @return 
	*/
	public void init() throws Exception {
        final HashMap<String, UsbDevice> usbList = mUsbManager.getDeviceList();
        if (usbList != null) {
            UsbDevice device;
            final Iterator<UsbDevice> iterator = usbList.values().iterator();
            while (iterator.hasNext()) {
                device = iterator.next();
                UsbInterface usbInterface = device.getInterface(0);
                if (usbInterface != null) {
                    if (device.getVendorId() == VID && device.getProductId() == PID) {
							//Log.d(TAG,"class:"+usbInterface.getInterfaceClass());
							mUsbDevice = device;
                    }
                }
            }
        }
		if (mUsbDevice == null) {
			Log.e(TAG, "UsbDevice is null");
			throw new Exception("UsbDevice is null");
		}
		//just need request permission and framework do grant always
		if (!mUsbManager.hasPermission(mUsbDevice)) {
			PendingIntent pi = PendingIntent.getBroadcast(mContext, 0, new Intent(ACTION_USB_PERMISSION), 0);
			mUsbManager.requestPermission(mUsbDevice, pi);
		}
		//get read,write ep
		UsbInterface usbIf = mUsbDevice.getInterface(1);
		for (int i = 0; i < usbIf.getEndpointCount(); i++) {
			UsbEndpoint end = usbIf.getEndpoint(i);
			if (end.getDirection() == UsbConstants.USB_DIR_IN) {
				mEpRead = end;
			} else {
				mEpWrite = end;
			}
		}
		//connect
		mUsbConnection = mUsbManager.openDevice(mUsbDevice);
		if (mUsbConnection != null) {
			if (!mUsbConnection.claimInterface(usbIf, true)) {
				Log.e(TAG, "claimInterface fail");
				throw new Exception("claimInterface fail");
			}
		} else {
			Log.e(TAG, "open fail");
			throw new Exception("open fail");
		}
		//init controller
		write(cmd_init, 2, 1000);//no ack
		write(cmd_get_id, 2, 1000);
		byte[] id = new byte[4];
		int len = read(id, 4, 1000);
		Log.d(TAG,"ID:"+bytes2Hex(id));//4 bytes id
	}

	/**
	* @brief check connection
	*
	* @return 
	*/
	public boolean isConnected() {
		if (mUsbConnection == null) {
			return false;
		} else {
			return true;
		}
	}

	/**
	* @brief release controller
	*
	* @return 
	*/
	public void release() throws Exception {
		if (mUsbConnection != null) {
			mUsbConnection.close();
			mUsbConnection = null;
		} else {
			throw new Exception("release fail");
		}
	}

	/**
	* @brief rotate motor
	*
	* @param index motor index 0-5
	*
	* @return 
	*/
	public void trigger(int index) throws Exception {
		Log.d(TAG,"cmd:"+bytes2Hex(cmd_motor_list[index]));
		write(cmd_motor_list[index], cmd_motor_list[index].length, 1000);
		byte[] ack = new byte[1];
		int len = read(ack, 1, 1000);
		Log.d(TAG,"ack:"+bytes2Hex(ack));// ack always 'S'
	}

	/**
	* @brief soft reset controller
	*
	* @return 
	*/
	public void reset() throws Exception {
		Log.d(TAG,"cmd:"+bytes2Hex(cmd_reset));
		write(cmd_reset, cmd_reset.length, 1000);
	}

	/**
	* @brief Write data to controller
	*
	* @param buf data buffer
	* @param len data len
	* @param timeout timeout in ms
	*
	* @return 
	*/
	private void write(byte[] buf, int len, int timeout) throws Exception {
		if (mUsbConnection != null && mEpWrite != null) {
			if(mUsbConnection.bulkTransfer(mEpWrite, buf, len, timeout) < 0) {
				throw new Exception("write fail");
			}
		} else {
			throw new Exception("no connect");
		}
	}

	/**
	* @brief Read data from controller
	*
	* @param buf data buffer
	* @param len data len
	* @param timeout timeout in ms
	*
	* @return 
	*/
	private int read(byte[] buf, int len, int timeout) throws Exception {
		int ret = 0;
		if (buf != null && mUsbConnection != null && mEpRead != null) {
			if((ret = mUsbConnection.bulkTransfer(mEpRead, buf, len, timeout)) < 0) {
				throw new Exception("read fail");
			}
		} else {
			throw new Exception("no connect");
		}
		return ret;
	}

	/**
	* @brief helper convert bytes to hex string
	*
	* @param src byte array
	*
	* @return 
	*/
	public static String bytes2Hex(byte[] src) {
		StringBuilder stringBuilder = new StringBuilder("");
		if (src == null || src.length <= 0) {
			return null;
		}
		char[] buffer = new char[2];
		for (byte b : src) {
			buffer[0] = Character.forDigit((b >>> 4) & 0x0F, 16);
			buffer[1] = Character.forDigit(b & 0x0F, 16);
			stringBuilder.append(" ");
			stringBuilder.append(buffer);
		}
		return stringBuilder.toString().toUpperCase();
	}  
}
