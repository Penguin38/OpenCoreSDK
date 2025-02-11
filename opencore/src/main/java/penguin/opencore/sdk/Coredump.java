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

    public static final String TAG = "Coredump";
    private static final Coredump sInstance = new Coredump();

    private static volatile boolean sReady = false;

    private static final int STANDBY = 0;
    private static final int COREDUMPED = 1 << 0;
    private static final int COREDUMP_COMPLETED = 1 << 1;
    private int mCondition = STANDBY;
    private final Object mLock = new Object();

    private OpencoreHandler mCoredumpWork;
    private OpencorePostHandler mCoredumpPostWork;
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

    public static final int FILTER_NONE = 0;
    public static final int FILTER_SPECIAL_VMA = 1 << 0;
    public static final int FILTER_FILE_VMA = 1 << 1;
    public static final int FILTER_SHARED_VMA = 1 << 2;
    public static final int FILTER_SANITIZER_SHADOW_VMA = 1 << 3;
    public static final int FILTER_NON_READ_VMA = 1 << 4;
    public static final int FILTER_SIGNAL_CONTEXT = 1 << 5;
    public static final int FILTER_MINIDUMP = 1 << 6;

    static {
        try {
            System.loadLibrary("opencore");
        } catch(UnsatisfiedLinkError e) {
            Log.e(TAG, "Cannot load opencore library", e);
        }
    }

    private Coredump() {}

    public static Coredump getInstance() {
        return sInstance;
    }

    public static boolean isReady() {
        return sReady;
    }

    public synchronized boolean init() {
        if (mCoredumpWork != null && mCoredumpPostWork != null)
            return sReady;

        try {
            if (!nativeInit())
                return false;

            if (mCoredumpWork == null) {
                HandlerThread core = new HandlerThread("opencore-bg");
                core.start();
                mCoredumpWork = new OpencoreHandler(core.getLooper());
            }

            if (mCoredumpPostWork == null) {
                HandlerThread post = new HandlerThread("opencore-post");
                post.start();
                mCoredumpPostWork = new OpencorePostHandler(post.getLooper());
            }

            sReady = true;
        } catch(UnsatisfiedLinkError e) {
            Log.e(TAG, "opencore init work fail", e);
        }
        return sReady;
    }

    public synchronized boolean enable(int type) {
        if (isReady()) {
            switch (type) {
                case JAVA:
                    return mJavaCrashHandler.enable();
                case NATIVE:
                    return nativeEnable();
            }
        }
        return false;
    }

    public synchronized boolean disable(int type) {
        if (isReady()) {
            switch (type) {
                case JAVA:
                    return mJavaCrashHandler.disable();
                case NATIVE:
                    return nativeDisable();
            }
        }
        return false;
    }

    public synchronized boolean isEnabled(int type) {
        if (isReady()) {
            switch (type) {
                case JAVA:
                    return mJavaCrashHandler.isEnabled();
                case NATIVE:
                    return nativeIsEnabled();
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

        mCondition &= ~COREDUMPED;
        sendEvent(CODE_COREDUMP, filename, mCoredumpWork);
        return waitCore();
    }

    private boolean waitCore() {
        try {
            synchronized (mLock) {
                while ((mCondition & COREDUMPED) != COREDUMPED)
                    mLock.wait();
            }
        } catch (InterruptedException e) {
            return false;
        }
        return true;
    }

    private void notifyCore() {
        synchronized (mLock) {
            mCondition |= COREDUMPED;
            mLock.notifyAll();
        }
    }

    private boolean doCompleted(String filename) {
         if (mListener == null)
            return true;

        mCondition &= ~COREDUMP_COMPLETED;
        sendEvent(CODE_COREDUMP_COMPLETED, filename, mCoredumpPostWork);
        return waitCompleted();
    }

    private boolean waitCompleted() {
        try {
            synchronized (mLock) {
                while ((mCondition & COREDUMP_COMPLETED) != COREDUMP_COMPLETED)
                    mLock.wait();
            }
        } catch (InterruptedException e) {
            return false;
        }
        return true;
    }

    private void notifyCompleted() {
        synchronized (mLock) {
            mCondition |= COREDUMP_COMPLETED;
            mLock.notifyAll();
        }
    }

    private void onCompleted(String filepath) {
        if (mListener != null) {
            mListener.onCompleted(filepath);
        }
    }

    public void setCoreDir(String dir) {
        if (isReady() && dir != null) {
            nativeSetDir(dir);
        }
    }

    public void setCoreFlag(int flag) {
        if (isReady()) {
            nativeSetFlag(flag);
        }
    }

    public void setCoreTimeout(int sec) {
        if (isReady()) {
            nativeSetTimeout(sec);
        }
    }

    public void setCoreFilter(int filter) {
        if (isReady()) {
            nativeSetFilter(filter);
        }
    }

    public String getCoreDir() {
        if (isReady()) {
            return nativeGetDir();
        }
        return "";
    }

    public int getCoreFlag() {
        if (isReady()) {
            return nativeGetFlag();
        }
        return 0;
    }

    public int getCoreTimeout() {
        if (isReady()) {
            return nativeGetTimeout();
        }
        return DEF_TIMEOUT;
    }

    public int getCoreFilter() {
        if (isReady()) {
            return nativeGetFilter();
        }
        return FILTER_NONE;
    }

    public String getVersion() {
        if (isReady())
            return nativeVersion();
        return "unknown";
    }

    private static native boolean nativeInit();
    private static native String nativeVersion();
    private static native boolean nativeEnable();
    private static native boolean nativeDisable();
    private static native boolean nativeCoredump(int tid, String filename);
    private static native void nativeSetDir(String dir);
    private static native void nativeSetFlag(int flag);
    private static native void nativeSetTimeout(int sec);
    private static native void nativeSetFilter(int filter);
    private static native boolean nativeIsEnabled();
    private static native String nativeGetDir();
    private static native int nativeGetFlag();
    private static native int nativeGetTimeout();
    private static native int nativeGetFilter();

    private static final int CODE_COREDUMP = 1;
    private static final int CODE_COREDUMP_COMPLETED = 2;

    private static class OpencoreHandler extends Handler {
        public OpencoreHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case CODE_COREDUMP: {
                    if (isReady()) {
                        if (msg.obj instanceof String) {
                            Coredump.getInstance().nativeCoredump(msg.arg1, (String)msg.obj);
                        } else {
                            Coredump.getInstance().nativeCoredump(msg.arg1, null);
                        }
                    }
                    // waitCore
                    Coredump.getInstance().notifyCore();
                } break;
            }
        }
    }

    private static class OpencorePostHandler extends Handler {
        public OpencorePostHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case CODE_COREDUMP_COMPLETED: {
                    if (msg.obj instanceof String) {
                        Coredump.getInstance().onCompleted((String)msg.obj);
                    } else {
                        Coredump.getInstance().onCompleted(null);
                    }
                    // waitCompleted
                    Coredump.getInstance().notifyCompleted();
                } break;
            }
        }
    }

    private void sendEvent(int code, String filename, Handler handler) {
        Message msg = Message.obtain(handler, code, Process.myTid(), 0, filename);
        msg.sendToTarget();
    }

    private static void callbackEvent(String filepath) {
        Coredump.getInstance().doCompleted(filepath);
    }

    public static interface Listener {
        void onCompleted(String path);
    }

    public void setListener(Listener listener) {
        mListener = listener;
    }

    private static class JavaCrashHandler implements Thread.UncaughtExceptionHandler {
        private Thread.UncaughtExceptionHandler defaultHandler;

        public boolean enable() {
            if (defaultHandler == null) {
                defaultHandler = Thread.getDefaultUncaughtExceptionHandler();
            }
            Thread.setDefaultUncaughtExceptionHandler(this);
            return true;
        }

        public boolean disable() {
            if (defaultHandler != null) {
                Thread.setDefaultUncaughtExceptionHandler(defaultHandler);
            }
            return true;
        }

        public boolean isEnabled() {
            return this == Thread.getDefaultUncaughtExceptionHandler();
        }

        @Override
        public void uncaughtException(Thread thread, Throwable throwable) {
            try {
                disable();
                Coredump.getInstance().doCoredump();
            } finally {
                Process.killProcess(Process.myPid());
                System.exit(10);
            }
        }
    }

    public static String coreFlagToString(int flag) {
        StringBuilder sb = new StringBuilder();
        if (flag == 0) {
            sb.append("FLAG_NONE");
            return sb.toString();
        }

        boolean need_seq = false;
        if ((flag & FLAG_CORE) != 0) {
            if (need_seq) sb.append('|');
            sb.append("FLAG_CORE");
            need_seq = true;
        }

        if ((flag & FLAG_PROCESS_COMM) != 0) {
            if (need_seq) sb.append('|');
            sb.append("FLAG_PROCESS_COMM");
            need_seq = true;
        }

        if ((flag & FLAG_PID) != 0) {
            if (need_seq) sb.append('|');
            sb.append("FLAG_PID");
            need_seq = true;
        }

        if ((flag & FLAG_THREAD_COMM) != 0) {
            if (need_seq) sb.append('|');
            sb.append("FLAG_THREAD_COMM");
            need_seq = true;
        }

        if ((flag & FLAG_TID) != 0) {
            if (need_seq) sb.append('|');
            sb.append("FLAG_TID");
            need_seq = true;
        }

        if ((flag & FLAG_TIMESTAMP) != 0) {
            if (need_seq) sb.append('|');
            sb.append("FLAG_TIMESTAMP");
            need_seq = true;
        }
        return sb.toString();
    }

    public static String coreFilterToString(int filter) {
        StringBuilder sb = new StringBuilder();
        if (filter == 0) {
            sb.append("FILTER_NONE");
            return sb.toString();
        }

        boolean need_seq = false;
        if ((filter & FILTER_SPECIAL_VMA) != 0) {
            if (need_seq) sb.append('|');
            sb.append("FILTER_SPECIAL_VMA");
            need_seq = true;
        }

        if ((filter & FILTER_FILE_VMA) != 0) {
            if (need_seq) sb.append('|');
            sb.append("FILTER_FILE_VMA");
            need_seq = true;
        }

        if ((filter & FILTER_SHARED_VMA) != 0) {
            if (need_seq) sb.append('|');
            sb.append("FILTER_SHARED_VMA");
            need_seq = true;
        }

        if ((filter & FILTER_SANITIZER_SHADOW_VMA) != 0) {
            if (need_seq) sb.append('|');
            sb.append("FILTER_SANITIZER_SHADOW_VMA");
            need_seq = true;
        }

        if ((filter & FILTER_NON_READ_VMA) != 0) {
            if (need_seq) sb.append('|');
            sb.append("FLAG_TID");
            need_seq = true;
        }

        if ((filter & FILTER_SIGNAL_CONTEXT) != 0) {
            if (need_seq) sb.append('|');
            sb.append("FILTER_SIGNAL_CONTEXT");
            need_seq = true;
        }

        if ((filter & FILTER_MINIDUMP) != 0) {
            if (need_seq) sb.append('|');
            sb.append("FILTER_MINIDUMP");
            need_seq = true;
        }
        return sb.toString();
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("Coredump[");
        if (!isReady()) {
            sb.append("NOT READY!!]");
            return sb.toString();
        }

        sb.append(nativeVersion());

        sb.append(",");
        sb.append(nativeGetDir());

        sb.append(",");
        sb.append(coreFlagToString(nativeGetFlag()));

        sb.append(",");
        sb.append(coreFilterToString(nativeGetFilter()));

        sb.append(",");
        sb.append(mJavaCrashHandler.isEnabled());

        sb.append(",");
        sb.append(nativeIsEnabled());

        sb.append(",");
        sb.append(nativeGetTimeout());

        sb.append("]");
        return sb.toString();
    }
}
