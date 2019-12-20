package com.paydevice.demo.motorcontrol;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbManager;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MotorControl";

    private MotorController mController;

    private final BroadcastReceiver mUsbStatusReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action != null) {
				if (action.equals(UsbManager.ACTION_USB_DEVICE_ATTACHED)) {
                    Log.d(TAG, "USB device attached");
					Toast.makeText(MainActivity.this, "USB device attached", Toast.LENGTH_SHORT).show();
					//wait for usb initialization
					try {
						Thread.sleep(1000);
					} catch (InterruptedException ie) {
					}
					connectController();
                } else {
                    Log.d(TAG, "USB device detached");
					Toast.makeText(MainActivity.this, "USB device deattached", Toast.LENGTH_SHORT).show();
                }
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }

    protected void onResume() {
        super.onResume();
        //register USB action
        IntentFilter filter = new IntentFilter();
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        registerReceiver(mUsbStatusReceiver, filter);
		connectController();
    }

    protected void onPause() {
        super.onPause();
        try {
            mController.release();
        } catch (Exception e) {
            Log.e(TAG, "Motor Controller close err");
        }
		unregisterReceiver(mUsbStatusReceiver);
    }

	private void connectController() {
		Log.e(TAG, "probe Motor Controller");
		try {
			if (mController == null) {
				mController = new MotorController(this);
			}
			mController.init();
		} catch (Exception e) {
			Log.e(TAG, "Motor Controller no found");
			Toast.makeText(this, "Motor Controller no found", Toast.LENGTH_SHORT).show();
		}
	}

    public void onBtnClick(View v) {
        try {
            mController.trigger(0);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void onBtn1Click(View v) {
        try {
            mController.trigger(1);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void onBtn2Click(View v) {
        try {
            mController.trigger(2);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void onBtn3Click(View v) {
        try {
            mController.trigger(3);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void onBtn4Click(View v) {
        try {
            mController.trigger(4);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void onBtn5Click(View v) {
        try {
            mController.trigger(5);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void onBtn6Click(View v) {
        try {
			if(mController.isConnected()) {
				//reset controller
				mController.reset();
				//release usb connection
				mController.release();
			}
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
