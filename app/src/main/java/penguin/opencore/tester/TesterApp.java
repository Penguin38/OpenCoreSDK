package penguin.opencore.tester;

import android.app.Application;

import penguin.opencore.sdk.Coredump;

public class TesterApp extends Application {
    @Override
    public void onCreate() {
        super.onCreate();

        Coredump.getInstance().init();

        Coredump.getInstance().setCoreMode(Coredump.MODE_PTRACE | Coredump.MODE_COPY);
        //Coredump.getInstance().setCoreMode(Coredump.MODE_PTRACE);
        //Coredump.getInstance().setCoreMode(Coredump.MODE_COPY);

        //Coredump.getInstance().setCoreDir(getFilesDir().getAbsolutePath());
        //Coredump.getInstance().setCoreDir("/data/local/tmp");
        Coredump.getInstance().setCoreDir(getExternalFilesDir(null).getAbsolutePath());

        Coredump.getInstance().enable(Coredump.JAVA);
        Coredump.getInstance().enable(Coredump.NATIVE);
    }
}
