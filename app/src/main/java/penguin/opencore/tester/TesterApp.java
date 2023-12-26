package penguin.opencore.tester;

import android.app.Application;

import penguin.opencore.sdk.Coredump;

public class TesterApp extends Application {
    @Override
    public void onCreate() {
        super.onCreate();

        Coredump.getInstance().init();

        Coredump.getInstance().setCoreTimeout(Coredump.DEF_TIMEOUT);

        //Coredump.getInstance().setCoreMode(Coredump.MODE_PTRACE | Coredump.MODE_COPY);
        //Coredump.getInstance().setCoreMode(Coredump.MODE_PTRACE);
        //Coredump.getInstance().setCoreMode(Coredump.MODE_COPY);
        Coredump.getInstance().setCoreMode(Coredump.MODE_COPY2);

        //Coredump.getInstance().setCoreDir(getFilesDir().getAbsolutePath());
        //Coredump.getInstance().setCoreDir("/data/local/tmp");
        Coredump.getInstance().setCoreDir(getExternalFilesDir(null).getAbsolutePath());
        Coredump.getInstance().setCoreFlag(Coredump.FLAG_CORE
                                         | Coredump.FLAG_PROCESS_COMM
                                         | Coredump.FLAG_PID
                                         | Coredump.FLAG_THREAD_COMM
                                         | Coredump.FLAG_TID
                                         | Coredump.FLAG_TIMESTAMP);

        Coredump.getInstance().enable(Coredump.JAVA);
        Coredump.getInstance().enable(Coredump.NATIVE);
    }
}
