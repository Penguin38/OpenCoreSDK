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

cmake_minimum_required(VERSION 3.10.2)
project("opencore")

file(READ ${CMAKE_SOURCE_DIR}/VERSION version_contents)
string(REGEX MATCH "MAJOR=([0-9]+)" _ ${version_contents})
set(MAJOR ${CMAKE_MATCH_1})
string(REGEX MATCH "MINOR=([0-9]+)" _ ${version_contents})
set(MINOR ${CMAKE_MATCH_1})
string(REGEX MATCH "PATCH=([0-9]+)" _ ${version_contents})
set(PATCH ${CMAKE_MATCH_1})
add_definitions(-D__OPENCORE_VERSION__="opencore-${MAJOR}.${MINOR}.${PATCH}")

include_directories(.)
include_directories(eajni/include/)
aux_source_directory(eajni SRC_LIST)
add_library(eajni STATIC ${SRC_LIST})
find_library(log-lib log)
target_link_libraries(eajni ${log-lib})

if (ANDROID_ABI STREQUAL "arm64-v8a")
    set(OPENCORE_IMPL
        opencore/lp64/opencore.cpp
        opencore/arm64/opencore.cpp)
elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
    set(OPENCORE_IMPL
        opencore/lp32/opencore.cpp
        opencore/arm/opencore.cpp)
elseif(ANDROID_ABI STREQUAL "armeabi")
    set(OPENCORE_IMPL
        opencore/lp32/opencore.cpp
        opencore/arm/opencore.cpp)
elseif(ANDROID_ABI STREQUAL "x86_64")
    set(OPENCORE_IMPL
        opencore/lp64/opencore.cpp
        opencore/x86_64/opencore.cpp)
elseif(ANDROID_ABI STREQUAL "x86")
    set(OPENCORE_IMPL
        opencore/lp32/opencore.cpp
        opencore/x86/opencore.cpp)
elseif(ANDROID_ABI STREQUAL "riscv64")
    set(OPENCORE_IMPL
        opencore/lp64/opencore.cpp
        opencore/riscv64/opencore.cpp)
endif()

add_library(opencore SHARED
            opencore/opencore.cpp
            ${OPENCORE_IMPL}
            opencore_jni.cpp)
target_link_libraries(opencore ${log-lib} eajni)
set_target_properties(opencore PROPERTIES LINK_FLAGS "-Wl,-z,max-page-size=16384")
