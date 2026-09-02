cmake_minimum_required(VERSION 3.22.1)
project(nether CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# android/app/src/main/cpp -> repository root
set(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../..")
set(SRC "${PROJECT_ROOT}/src")

# Kept as an explicit list (mirroring the desktop Makefile) rather than a glob
# so that adding a new desktop-only backend never silently lands in the APK.
set(GAME_SOURCES
    3dobject-ase.cpp
    3dobject.cpp
    bitmap.cpp
    bullet.cpp
    cmc.cpp
    construction.cpp
    enemy_ai.cpp
    glport.cpp
    glprintf.cpp
    main.cpp
    mainmenu.cpp
    maps.cpp
    menu.cpp
    myglutaux.cpp
    nether.cpp
    nethercycle.cpp
    netherdebug.cpp
    nethersave.cpp
    particles.cpp
    piece3dobject.cpp
    quaternion.cpp
    radar.cpp
    robot_ai.cpp
    robots.cpp
    shadow3dobject.cpp
    vector.cpp
)

# Android replacements for platform.cpp / audio.cpp / assets.cpp (all three of
# which are SDL/desktop-only), plus the JNI glue.
set(ANDROID_SOURCES
    platform_android.cpp
    audio_android.cpp
    assets_android.cpp
    android_bridge.cpp
)

list(TRANSFORM GAME_SOURCES    PREPEND "${SRC}/")
list(TRANSFORM ANDROID_SOURCES PREPEND "${SRC}/")

add_library(nether SHARED ${GAME_SOURCES} ${ANDROID_SOURCES})

target_include_directories(nether PRIVATE "${SRC}")

# Only the JNI entry points (JNIEXPORT) need to be visible to the loader.
target_compile_options(nether PRIVATE -fvisibility=hidden -ffunction-sections -fdata-sections)
target_link_options(nether PRIVATE -Wl,--gc-sections)

find_library(log-lib log)
find_library(android-lib android)
find_library(egl-lib EGL)
find_library(gles-lib GLESv1_CM)

target_link_libraries(nether
    ${log-lib}
    ${android-lib}
    ${egl-lib}
    ${gles-lib}
)
