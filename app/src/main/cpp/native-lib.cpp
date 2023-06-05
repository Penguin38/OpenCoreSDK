#include <jni.h>
#include <string>

extern "C"
JNIEXPORT void JNICALL
Java_penguin_opencore_tester_MainActivity_nativeCrashJNI(JNIEnv *env, jobject thiz) {
    int *p = nullptr;
    *p = 1;
}
