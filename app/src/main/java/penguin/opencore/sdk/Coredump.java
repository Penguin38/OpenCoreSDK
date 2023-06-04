package penguin.opencore.sdk;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.Process;

import androidx.annotation.NonNull;

public class Coredump {

    private static final String TAG = "Coredump";
    private static final Coredump sInstance = new Coredump();

    private static boolean sIsInit = false;
    private OpencoreHandler mCoredumpWork;
    private final Object mLock = new Object();
    private String mCoreDir;
    private Listener mListener;
    private JavaCrashHandler mJavaCrashHandler = new JavaCrashHandler();

    public static final int JAVA = 1;
    public static final int NATIVE = 2;

    static {
        System.loadLibrary("opencore");
    }

    private Coredump() {}

    public static synchronized Coredump getInstance() {
        return sInstance;
    }

    public synchronized boolean init() {
        if (!sIsInit) {
            HandlerThread core = new HandlerThread("opencore-bg");
            core.start();
            mCoredumpWork = new OpencoreHandler(core.getLooper());
            sIsInit = true;
            return true;
        }
        throw new IllegalStateException();
    }

    public synchronized boolean enable(int type) {
        if (!sIsInit)
            return false;

        switch (type) {
            case JAVA:
                return mJavaCrashHandler.enableJavaCrash();
            case NATIVE:
                return native_enable();
        }
        return false;
    }

    public synchronized boolean disable(int type) {
        if (!sIsInit)
            return false;

        switch (type) {
            case JAVA:
                return mJavaCrashHandler.disableJavaCrash();
            case NATIVE:
                return native_diable();
        }
        return false;
    }

    public synchronized boolean doCoredump() {
        if (!sIsInit)
            return false;

        sendEvent(OpencoreHandler.CODE_COREDUMP);
        return waitCore();
    }

    public boolean waitCore() {
        synchronized (mLock) {
            try {
                mLock.wait();
            } catch (InterruptedException e) {
                return false;
            }
        }
        return true;
    }

    private void onCompleted() {
        synchronized (mLock) {
            mLock.notify();
        }
        if (mListener != null) {
            mListener.onCompleted(mCoreDir + "/core." + Process.myPid());
        }
    }

    public void setCoreDir(String dir) {
        mCoreDir = dir;
        native_setCoreDir(dir);
    }

    public native String getVersion();
    private native boolean native_enable();
    private native boolean native_diable();
    private native boolean native_doCoredump();
    private native void native_setCoreDir(String dir);

    private static class OpencoreHandler extends Handler {
        public static final int CODE_COREDUMP = 1;
        public static final int CODE_COREDUMP_COMPLETED = 2;

        public OpencoreHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case CODE_COREDUMP:
                    Coredump.getInstance().native_doCoredump();
                    break;
                case CODE_COREDUMP_COMPLETED:
                    Coredump.getInstance().onCompleted();
                    break;
            }
        }
    }

    private void sendEvent(int code) {
        Message msg = Message.obtain(mCoredumpWork, code);
        msg.sendToTarget();
    }

    private static void callbackEvent() {
        Coredump.getInstance().sendEvent(OpencoreHandler.CODE_COREDUMP_COMPLETED);
    }

    public static interface Listener {
        void onCompleted(String dir);
    }

    public void setListener(Listener listener) {
        mListener = listener;
    }

    private static class JavaCrashHandler implements Thread.UncaughtExceptionHandler {
        private Thread.UncaughtExceptionHandler defaultHandler;

        public boolean enableJavaCrash() {
            defaultHandler = Thread.getDefaultUncaughtExceptionHandler();
            Thread.setDefaultUncaughtExceptionHandler(this);
            return true;
        }

        public boolean disableJavaCrash() {
            if (defaultHandler != null) {
                Thread.setDefaultUncaughtExceptionHandler(defaultHandler);
            }
            return true;
        }

        @Override
        public void uncaughtException(@NonNull Thread thread, @NonNull Throwable throwable) {
            disableJavaCrash();
            Coredump.getInstance().doCoredump();
            Process.killProcess(Process.myPid());
        }
    }
}
