# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

project("sdk")

include_directories(EAJNI/include/)
aux_source_directory(EAJNI SRC_LIST)
add_library(eajni STATIC ${SRC_LIST})
find_library(log-lib log)
target_link_libraries(eajni ${log-lib})

if (ANDROID_ABI STREQUAL "arm64-v8a")
    set(OPENCORE_IMPL arm64/opencore_impl.cpp)
elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
    set(OPENCORE_IMPL arm/opencore_impl.cpp)
elseif(ANDROID_ABI STREQUAL "armeabi")
    set(OPENCORE_IMPL arm/opencore_impl.cpp)
endif()

add_library(opencore SHARED opencore.cpp ${OPENCORE_IMPL} opencore_jni.cpp)
target_link_libraries(opencore ${log-lib} eajni)

add_library(opencore-jni SHARED opencore_jni.cpp)
target_link_libraries(opencore-jni opencore ${log-lib} eajni)