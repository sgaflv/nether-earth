apk_release := "android/app/build/outputs/apk/release/app-release.apk"

# Desktop (SDL2) build.
build:
    make -j

# Android TV / armeabi-v7a APK, native code optimised (needs ANDROID_HOME).
# Produces nether-earth.apk in the project root.
apk:
    cd android && ./gradlew :app:assembleRelease
    cp {{apk_release}} nether-earth.apk
    @ls -l nether-earth.apk

# Same, but a debuggable variant with unoptimised native code.
apk-debug:
    cd android && ./gradlew :app:assembleDebug

apk-clean:
    cd android && ./gradlew clean

connect:
    adb connect 192.168.188.54

install apk=apk_release:
    adb install -r -d {{apk}}

# Build, install and launch in one go.
deploy: apk
    adb install -r -d {{apk_release}}
    just run

# Live log: the game's own tag, plus crashes and EGL/activity events.
logs:
    adb logcat -c
    adb logcat -s NetherEarth:V AndroidRuntime:E DEBUG:V libEGL:V ActivityManager:I

# Everything the device has buffered since boot for this app, no filtering.
logs-dump:
    adb logcat -d -v time | grep -iE "nether|AndroidRuntime|libEGL|DEBUG" | tail -200

# The native/Java crash buffer, if the app died rather than exited.
logs-crash:
    adb logcat -b crash -d -v time | tail -100

run game="nether":
    adb shell monkey -p $(adb shell pm list packages | grep -i "{{game}}" | head -n1 | sed 's/^package://' | tr -d '\r') 1

stop game="nether":
    adb shell pm list packages | grep -i "{{game}}"
    adb shell am force-stop $(adb shell pm list packages | grep -i "{{game}}" | head -n1 | cut -d: -f2 | tr -d '\r')

remove game="nether":
    adb uninstall $(adb shell pm list packages | grep -i "{{game}}" | head -n1 | cut -d: -f2 | tr -d '\r')
