apk_release := "android/app/build/outputs/apk/release/app-release.apk"

# Desktop (SDL2) build.
build:
    make -j

# Android TV / armeabi-v7a APK, native code optimised (needs ANDROID_HOME)
apk:
    cd android && ./gradlew :app:assembleRelease
    @ls -l {{apk_release}}

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

logs:
    adb logcat -s NetherEarth:V AndroidRuntime:E DEBUG:V

run game="nether":
    adb shell monkey -p $(adb shell pm list packages | grep -i "{{game}}" | head -n1 | sed 's/^package://' | tr -d '\r') 1

stop game="nether":
    adb shell pm list packages | grep -i "{{game}}"
    adb shell am force-stop $(adb shell pm list packages | grep -i "{{game}}" | head -n1 | cut -d: -f2 | tr -d '\r')

remove game="nether":
    adb uninstall $(adb shell pm list packages | grep -i "{{game}}" | head -n1 | cut -d: -f2 | tr -d '\r')
