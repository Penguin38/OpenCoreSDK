package penguin.opencore.sdk;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.Process;

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

    private int mCoreMode = MODE_COPY2;
    public static final int MODE_PTRACE = 1 << 0;
    public static final int MODE_COPY = 1 << 1;
    public static final int MODE_COPY2 = 1 << 2;
    public static final int MODE_MAX = MODE_COPY2;

    private int mFlag = FLAG_CORE | FLAG_TID;
    public static final int FLAG_CORE = 1 << 0;
    public static final int FLAG_PROCESS_COMM = 1 << 1;
    public static final int FLAG_PID = 1 << 2;
    public static final int FLAG_THREAD_COMM = 1 << 3;
    public static final int FLAG_TID = 1 << 4;
    public static final int FLAG_TIMESTAMP = 1 << 5;

    static {
        System.loadLibrary("opencore-jni");
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
        return doCoredump(null);
    }

    public synchronized boolean doCoredump(String filename) {
        if (!sIsInit)
            return false;

        sendEvent(OpencoreHandler.CODE_COREDUMP, filename);
        return waitCore();
    }

    private boolean waitCore() {
        synchronized (mLock) {
            try {
                mLock.wait();
            } catch (InterruptedException e) {
                return false;
            }
        }
        return true;
    }

    private void onCompleted(String filepath) {
        synchronized (mLock) {
            mLock.notify();
        }
        if (mListener != null) {
            mListener.onCompleted(filepath);
        }
    }

    public void setCoreDir(String dir) {
        if (dir != null) {
            mCoreDir = dir;
            native_setCoreDir(dir);
        }
    }

    public void setCoreMode(int mode) {
        if (mode > MODE_MAX) {
            mCoreMode = MODE_MAX;
        } else {
            mCoreMode = mode;
        }
        native_setCoreMode(mCoreMode);
    }

    public void setCoreFlag(int flag) {
        mFlag = flag;
        native_setCoreFlag(mFlag);
    }

    public native String getVersion();
    private native boolean native_enable();
    private native boolean native_diable();
    private native boolean native_doCoredump(String filename);
    private native void native_setCoreDir(String dir);
    private native void native_setCoreMode(int mode);
    private native void native_setCoreFlag(int flag);

    private static class OpencoreHandler extends Handler {
        public static final int CODE_COREDUMP = 1;
        public static final int CODE_COREDUMP_COMPLETED = 2;

        public OpencoreHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case CODE_COREDUMP: {
                    if (msg.obj instanceof String) {
                        Coredump.getInstance().native_doCoredump((String)msg.obj);
                    } else {
                        Coredump.getInstance().native_doCoredump(null);
                    }
                } break;
                case CODE_COREDUMP_COMPLETED: {
                    if (msg.obj instanceof String) {
                        Coredump.getInstance().onCompleted((String)msg.obj);
                    } else {
                        Coredump.getInstance().onCompleted(null);
                    }
                } break;
            }
        }
    }

    private void sendEvent(int code) {
        sendEvent(code, null);
    }

    private void sendEvent(int code, String filename) {
        Message msg = Message.obtain(mCoredumpWork, code, filename);
        msg.sendToTarget();
    }

    private static void callbackEvent(String filepath) {
        Coredump.getInstance().sendEvent(OpencoreHandler.CODE_COREDUMP_COMPLETED, filepath);
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
            if (defaultHandler == null) {
                defaultHandler = Thread.getDefaultUncaughtExceptionHandler();
            }
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
        public void uncaughtException(Thread thread, Throwable throwable) {
            disableJavaCrash();
            Coredump.getInstance().doCoredump();
            Process.killProcess(Process.myPid());
        }
    }
}
