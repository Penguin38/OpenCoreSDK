package penguin.opencore.tester;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;
import penguin.opencore.sdk.Coredump;

public class MainActivity extends AppCompatActivity
        implements View.OnClickListener, Coredump.Listener {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Coredump.getInstance().setListener(this);
        findViewById(R.id.button).setOnClickListener(this);
        findViewById(R.id.button2).setOnClickListener(this);
        findViewById(R.id.button3).setOnClickListener(this);
        findViewById(R.id.button4).setOnClickListener(this);
    }

    private void doJavaCrash() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                View view = null;
                view.callOnClick();
            }
        }).start();
    }

    private void doNativeCrash() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                nativeCrashJNI();
            }
        }).start();
    }

    private void doDirectJavaCore() {
        Coredump.getInstance().doCoredump();
    }

    private void doAbort() {
         new Thread(new Runnable() {
            @Override
            public void run() {
                nativeAbortJNI();
            }
        }).start();
    }

    @Override
    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.button:
                doJavaCrash();
                break;
            case R.id.button2:
                doNativeCrash();
                break;
            case R.id.button3:
                doDirectJavaCore();
                break;
            case R.id.button4:
                doAbort();
                break;
        }
    }

    @Override
    public void onCompleted(String path) {
        Toast.makeText(MainActivity.this, path, Toast.LENGTH_LONG).show();
    }
    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native void nativeCrashJNI();
    public native void nativeAbortJNI();
}
