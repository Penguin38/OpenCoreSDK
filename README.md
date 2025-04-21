# Techincal System
![core-analysis](https://raw.githubusercontent.com/Penguin38/OpenCoreAnalysisKit/refs/heads/main/doc/OpenCoreAnalyzer.jpg)

| Project      | Path                                              |
|:------------:|---------------------------------------------------|
|core-parser   | https://github.com/Penguin38/OpenCoreAnalysisKit  |
|linux-parser  | https://github.com/Penguin38/OpenLinuxAnalysisKit |
|crash-android | https://github.com/Penguin38/crash-android        |
|OpenCoreSDK   | https://github.com/Penguin38/OpenCoreSDK          |

## Build
```
allprojects {
    repositories {
        ...
        maven { url 'https://jitpack.io' }
    }
}
```
or
```
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        ...
        maven { url 'https://jitpack.io' }
    }
}
```
```
dependencies {
    ...
    implementation 'com.github.Penguin38:OpenCoreSDK:opencore-1.4.15'
}
```
## Simple
```
{
    //  init opencore env
    Coredump.getInstance().init();

    //  setting timeout (second)
    Coredump.getInstance().setCoreTimeout(Coredump.DEF_TIMEOUT);

    //  setting core filename rule
    Coredump.getInstance().setCoreFlag(Coredump.FLAG_CORE
                                     | Coredump.FLAG_PROCESS_COMM
                                     | Coredump.FLAG_PID
                                     | Coredump.FLAG_THREAD_COMM
                                     | Coredump.FLAG_TID
                                     | Coredump.FLAG_TIMESTAMP);
    
    //  setting core filter
    Coredump.getInstance().setCoreFilter(Coredump.FILTER_SPECIAL_VMA
                                      // | Coredump.FILTER_FILE_VMA
                                      // | Coredump.FILTER_SHARED_VMA
                                       | Coredump.FILTER_SANITIZER_SHADOW_VMA
                                       | Coredump.FILTER_NON_READ_VMA
                                       | Coredump.FILTER_SIGNAL_CONTEXT
                                      // | Coredump.FILTER_JAVAHEAP_VMA
                                      // | Coredump.FILTER_JIT_CACHE_VMA
                                      /* | Coredump.FILTER_MINIDUMP */);

    //  setting core save dir
    Coredump.getInstance().setCoreDir(...);
   
    //  Java Crash
    Coredump.getInstance().enable(Coredump.JAVA);
    
    //  Native Crash
    Coredump.getInstance().enable(Coredump.NATIVE);
    
    //  setting core listener
    Coredump.getInstance().setListener(new Coredump.Listener() {
        @Override
        public void onCompleted(String path) {
                // do anything
        }
    });

    //  current time core
    Coredump.getInstance().doCoredump();
}
```
```
01-12 00:15:33.949 28421 28421 I Opencore: Init opencore-1.4.7 environment..
01-12 00:15:35.479 28471 28471 I Opencore: Coredump /storage/emulated/0/Android/data/penguin.opencore.tester/files/core.opencore.tester_28421_Thread-2_28470_1736612135 ...
01-12 00:15:35.479 28421 28470 I Opencore: Wait (28471) coredump
01-12 00:15:37.927 28471 28471 I Opencore: Finish done.
```

```
$ core-parser -c core.opencore.tester_28421_Thread-2_28470_1736612135

core-parser> sysroot symbols
Mmap segment [5c8d0ba000, 5c8d0bc000) symbols/system/bin/app_process64 [0]
Mmap segment [5c8d0bc000, 5c8d0bd000) symbols/system/bin/app_process64 [2000]
Read symbols[18] (/system/bin/app_process64)
Mmap segment [717a200000, 717a34a000) symbols/apex/com.android.art/lib64/libart.so [0]
Mmap segment [717a400000, 717a9d7000) symbols/apex/com.android.art/lib64/libart.so [200000]
Read symbols[11831] (/apex/com.android.art/lib64/libart.so)
Mmap segment [714eee9000, 714eeea000) symbols/data/app/~~t-3m0JHw8Ez4qv9bddEVKg==/penguin.opencore.tester-rYieubZRUEwwTqkECUNBYQ==/lib/arm64/libnative-lib.so [0]
Read symbols[10] (/data/app/~~t-3m0JHw8Ez4qv9bddEVKg==/penguin.opencore.tester-rYieubZRUEwwTqkECUNBYQ==/lib/arm64/libnative-lib.so)

core-parser> bt
Switch oat version(199) env.
"Thread-2" sysTid=28470 Native
  | group="main" daemon=0 prio=5 target=0x131e9438 uncaught_exception=0x0
  | tid=28 sCount=0 flags=0 obj=0x131e93c0 self=0xb4000070fb248800 env=0xb4000070fb110380
  | stack=0x70e1e41000-0x70e1e43000 stackSize=0x103cb0 handle=0x70e1f44cb0
  | mutexes=0xb4000070fb248fa8 held=
  x0  0xb4000070fb110380  x1  0x00000070e1f43f68  x2  0x0000000000000000  x3  0x0000000000000000
  x4  0x0000000000000000  x5  0x0000000000000000  x6  0x0000000000000000  x7  0x0000000000000000
  x8  0x0000000000000000  x9  0x0000000000000001  x10 0x0000000000430000  x11 0x0000007140000000
  x12 0x0000000000000000  x13 0x0000000000000228  x14 0x0092b9b7e4df6987  x15 0xffffffffffffffff
  x16 0x0000007204fced50  x17 0x000000714eee95f0  x18 0x00000070e0fd0000  x19 0xb4000070fb248800
  x20 0x0000000000000000  x21 0xb4000070fb248800  x22 0x00000070e1f45000  x23 0xb4000070fb2488b0
  x24 0x0000007158b31a30  x25 0xb40000721ece1000  x26 0x00000070e1f45000  x27 0x000000000000000b
  x28 0x00000070e1f43e80  fp  0x00000070e1f43e80
  lr  0x000000717a422248  sp  0x00000070e1f43e50  pc  0x000000714eee9608  pst 0x0000000060001000
  Native: #0  000000714eee9608  Java_penguin_opencore_tester_MainActivity_nativeCrashJNI+0x18
  Native: #1  000000717a422244  art_quick_generic_jni_trampoline+0x94
  JavaKt: #0  0000000000000000  penguin.opencore.tester.MainActivity.nativeCrashJNI
  JavaKt: #1  0000007165277b30  penguin.opencore.tester.MainActivity$2.run
  JavaKt: #2  0000007179e13eb4  java.lang.Thread.run
core-parser>
```

```
$ gdb16 -c core.opencore.tester_28421_Thread-2_28470_1736612135

(gdb) set sysroot symbols/
Reading symbols from symbols/apex/com.android.art/lib64/libart.so...
(No debugging symbols found in symbols/apex/com.android.art/lib64/libart.so)
Reading symbols from symbols/data/app/~~t-3m0JHw8Ez4qv9bddEVKg==/penguin.opencore.tester-rYieubZRUEwwTqkECUNBYQ==/lib/arm64/libnative-lib.so...

(gdb) bt
#0  0x000000714eee9608 in Java_penguin_opencore_tester_MainActivity_nativeCrashJNI (env=0xb4000070fb110380, thiz=0x70e1f43f68)
    at /home/penguin/opensource/OpenCoreSDK/app/src/main/cpp/native-lib.cpp:25
#1  0x000000717a422248 in art_quick_generic_jni_trampoline () from symbols/apex/com.android.art/lib64/libart.so
#2  0x000000717a418968 in art_quick_invoke_stub () from symbols/apex/com.android.art/lib64/libart.so
#3  0x000000717a484064 in art::ArtMethod::Invoke(art::Thread*, unsigned int*, unsigned int, art::JValue*, char const*) () from symbols/apex/com.android.art/lib64/libart.so
#4  0x000000717a5e1734 in art::interpreter::ArtInterpreterToCompiledCodeBridge(art::Thread*, art::ArtMethod*, art::ShadowFrame*, unsigned short, art::JValue*) ()
   from symbols/apex/com.android.art/lib64/libart.so
#5  0x000000717a5dc028 in bool art::interpreter::DoCall<false, false>(art::ArtMethod*, art::Thread*, art::ShadowFrame&, art::Instruction const*, unsigned short, art::JValue*) ()
   from symbols/apex/com.android.art/lib64/libart.so
#6  0x000000717a943a68 in MterpInvokeVirtual () from symbols/apex/com.android.art/lib64/libart.so
#7  0x000000717a403818 in mterp_op_invoke_virtual () from symbols/apex/com.android.art/lib64/libart.so
#8  0x000000717a5d3fd4 in art::interpreter::Execute(art::Thread*, art::CodeItemDataAccessor const&, art::ShadowFrame&, art::JValue, bool, bool) ()
   from symbols/apex/com.android.art/lib64/libart.so
#9  0x000000717a5db5a8 in art::interpreter::ArtInterpreterToInterpreterBridge(art::Thread*, art::CodeItemDataAccessor const&, art::ShadowFrame*, art::JValue*) ()
   from symbols/apex/com.android.art/lib64/libart.so
#10 0x000000717a5dc004 in bool art::interpreter::DoCall<false, false>(art::ArtMethod*, art::Thread*, art::ShadowFrame&, art::Instruction const*, unsigned short, art::JValue*) ()
   from symbols/apex/com.android.art/lib64/libart.so
#11 0x000000717a949c00 in MterpInvokeInterface () from symbols/apex/com.android.art/lib64/libart.so
#12 0x000000717a403a18 in mterp_op_invoke_interface () from symbols/apex/com.android.art/lib64/libart.so
#13 0x000000717a5d3fd4 in art::interpreter::Execute(art::Thread*, art::CodeItemDataAccessor const&, art::ShadowFrame&, art::JValue, bool, bool) ()
   from symbols/apex/com.android.art/lib64/libart.so
#14 0x000000717a932568 in artQuickToInterpreterBridge () from symbols/apex/com.android.art/lib64/libart.so
#15 0x000000717a42237c in art_quick_to_interpreter_bridge () from symbols/apex/com.android.art/lib64/libart.so
#16 0x000000717a418968 in art_quick_invoke_stub () from symbols/apex/com.android.art/lib64/libart.so
#17 0x000000717a484064 in art::ArtMethod::Invoke(art::Thread*, unsigned int*, unsigned int, art::JValue*, char const*) () from symbols/apex/com.android.art/lib64/libart.so
#18 0x000000717a818618 in art::JValue art::InvokeVirtualOrInterfaceWithJValues<art::ArtMethod*>(art::ScopedObjectAccessAlreadyRunnable const&, _jobject*, art::ArtMethod*, jvalue const*) () from symbols/apex/com.android.art/lib64/libart.so
#19 0x000000717a866b24 in art::Thread::CreateCallback(void*) () from symbols/apex/com.android.art/lib64/libart.so
...
```
