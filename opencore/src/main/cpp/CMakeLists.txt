#
# Copyright (C) 2024-present, Guanyou.Chen. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
elseif(ANDROID_ABI STREQUAL "x86_64")
    set(OPENCORE_IMPL x86_64/opencore_impl.cpp)
elseif(ANDROID_ABI STREQUAL "x86")
    set(OPENCORE_IMPL x86/opencore_impl.cpp)
endif()

add_library(opencore SHARED opencore.cpp ${OPENCORE_IMPL} opencore_jni.cpp)
target_link_libraries(opencore ${log-lib} eajni)

#add_library(opencore-jni SHARED opencore_jni.cpp)
#target_link_libraries(opencore-jni opencore ${log-lib} eajni)
