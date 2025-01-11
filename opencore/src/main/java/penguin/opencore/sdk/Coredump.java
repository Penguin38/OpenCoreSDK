/*
 * Copyright (C) 2024-present, Guanyou.Chen. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package penguin.opencore.sdk;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.Process;
import android.util.Log;

public class Coredump {

    private static final String TAG = "Coredump";
    private static final Coredump sInstance = new Coredump();

    private static volatile boolean sLoaded = false;
    private static volatile boolean sIsInit = false;
    private static volatile boolean sReady = false;

    private OpencoreHandler mCoredumpWork;
    private final Object mLock = new Object();
    private Listener mListener;
    private JavaCrashHandler mJavaCrashHandler = new JavaCrashHandler();

    public static final int JAVA = 1;
    public static final int NATIVE = 2;

    public static final int FLAG_CORE = 1 << 0;
    public static final int FLAG_PROCESS_COMM = 1 << 1;
    public static final int FLAG_PID = 1 << 2;
    public static final int FLAG_THREAD_COMM = 1 << 3;
    public static final int FLAG_TID = 1 << 4;
    public static final int FLAG_TIMESTAMP = 1 << 5;

    public static final int DEF_TIMEOUT = 120;

    public static final int FILTER_NONE = 0x0;
    public static final int FILTER_SPECIAL_VMA = 1 << 0;
    public static final int FILTER_FILE_VMA = 1 << 1;
    public static final int FILTER_SHARED_VMA = 1 << 2;
    public static final int FILTER_SANITIZER_SHADOW_VMA = 1 << 3;
    public static final int FILTER_NON_READ_VMA = 1 << 4;
    public static final int FILTER_SIGNAL_CONTEXT = 1 << 5;

    static {
        try {
            System.loadLibrary("opencore");
            sLoaded = true;
        } catch(UnsatisfiedLinkError e) {
            Log.e(TAG, "Can't load opencore-jni", e);
        }
    }

    private Coredump() {}

    public static synchronized Coredump getInstance() {
        return sInstance;
    }

    public static boolean isReady() {
        return sReady;
    }

    public synchronized boolean init() {
        if (sLoaded) {
            if (!sIsInit) {
                HandlerThread core = new HandlerThread("opencore-bg");
                core.start();
                mCoredumpWork = new OpencoreHandler(core.getLooper());
                sReady = native_init(this);
                sIsInit = true;
            } else {
                Log.i(TAG, "Already init opencore.");
            }
        }
        return sIsInit;
    }

    public synchronized boolean enable(int type) {
        if (isReady()) {
            switch (type) {
                case JAVA:
                    return mJavaCrashHandler.enableJavaCrash();
                case NATIVE:
                    return native_enable();
            }
        }
        return false;
    }

    public synchronized boolean disable(int type) {
        if (isReady()) {
            switch (type) {
                case JAVA:
                    return mJavaCrashHandler.disableJavaCrash();
                case NATIVE:
                    return native_disable();
            }
        }
        return false;
    }

    public synchronized boolean doCoredump() {
        return doCoredump(null);
    }

    public synchronized boolean doCoredump(String filename) {
        if (!isReady())
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

    private void notifyCore() {
        synchronized (mLock) {
            mLock.notify();
        }
    }

    private void onCompleted(String filepath) {
        if (mListener != null) {
            mListener.onCompleted(filepath);
        }
    }

    public void setCoreDir(String dir) {
        if (isReady() && dir != null) {
            native_setCoreDir(dir);
        }
    }

    public void setCoreFlag(int flag) {
        if (isReady()) {
            native_setCoreFlag(flag);
        }
    }

    public void setCoreTimeout(int sec) {
        if (isReady()) {
            native_setCoreTimeout(sec);
        }
    }

    public void setCoreFilter(int filter) {
        if (isReady()) {
            native_setCoreFilter(filter);
        }
    }

    private static native boolean native_init(Coredump coredump);
    public native String getVersion();
    private native boolean native_enable();
    private native boolean native_disable();
    private native boolean native_doCoredump(int tid, String filename);
    private native void native_setCoreDir(String dir);
    private native void native_setCoreFlag(int flag);
    private native void native_setCoreTimeout(int sec);
    private native void native_setCoreFilter(int filter);

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
                    if (isReady()) {
                        if (msg.obj instanceof String) {
                            Coredump.getInstance().native_doCoredump(msg.arg1, (String)msg.obj);
                        } else {
                            Coredump.getInstance().native_doCoredump(msg.arg1, null);
                        }
                    }
                    Coredump.getInstance().notifyCore();
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
        Message msg = Message.obtain(mCoredumpWork, code, Process.myTid(), 0, filename);
        msg.sendToTarget();
    }

    private static void callbackEvent(String filepath) {
        Coredump.getInstance().sendEvent(OpencoreHandler.CODE_COREDUMP_COMPLETED, filepath);
    }

    public static interface Listener {
        void onCompleted(String path);
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
            try {
                disableJavaCrash();
                Coredump.getInstance().doCoredump();
            } finally {
                Process.killProcess(Process.myPid());
                System.exit(10);
            }
        }
    }
}
