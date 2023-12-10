#include <jni.h>
#include <string>
#include <stdlib.h>

extern "C"
JNIEXPORT void JNICALL
Java_penguin_opencore_tester_MainActivity_nativeCrashJNI(JNIEnv *env, jobject thiz) {
    int *p = nullptr;
    *p = 1;
}

extern "C"
JNIEXPORT void JNICALL
Java_penguin_opencore_tester_MainActivity_nativeAbortJNI(JNIEnv *env, jobject thiz) {
    abort();
}
