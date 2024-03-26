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

        Coredump.getInstance().setCoreFilter(Coredump.FILTER_SPECIAL_VMA
                                          // | Coredump.FILTER_FILE_VMA
                                          // | Coredump.FILTER_SHARED_VMA
                                           | Coredump.FILTER_SANITIZER_SHADOW_VMA
                                           | Coredump.FILTER_NON_READ_VMA);

        Coredump.getInstance().enable(Coredump.JAVA);
        Coredump.getInstance().enable(Coredump.NATIVE);
    }
}
