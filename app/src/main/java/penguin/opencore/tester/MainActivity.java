package penguin.opencore.tester;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;
import penguin.opencore.sdk.Coredump;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Coredump.getInstance().init();
        //Coredump.getInstance().setCoreDir(getFilesDir().getAbsolutePath());
        //Coredump.getInstance().setCoreDir("/data/local/tmp");
        Coredump.getInstance().setCoreDir(getExternalFilesDir(null).getAbsolutePath());
        Coredump.getInstance().setListener(new Coredump.Listener() {
            @Override
            public void onCompleted(String path) {
                Toast.makeText(MainActivity.this, path, Toast.LENGTH_LONG).show();
            }
        });
        Coredump.getInstance().enable(Coredump.JAVA);
        Coredump.getInstance().enable(Coredump.NATIVE);

        Button javaCrash = findViewById(R.id.button);
        javaCrash.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        View view = null;
                        view.callOnClick();
                    }
                }).start();
            }
        });

        Button nativeCrash = findViewById(R.id.button2);
        nativeCrash.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        nativeCrashJNI();
                    }
                }).start();
            }
        });

        Button directJavaCore = findViewById(R.id.button3);
        directJavaCore.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Coredump.getInstance().doCoredump();
            }
        });
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native void nativeCrashJNI();
}
