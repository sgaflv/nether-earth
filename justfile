connect:
    adb connect 192.168.188.54

install apk="nether-earth.apk":
    adb install -r -d {{apk}}

run game="nether":
    adb shell monkey -p $(adb shell pm list packages | grep -i "{{game}}" | head -n1 | sed 's/^package://' | tr -d '\r') 1

stop game="nether":
    adb shell pm list packages | grep -i "{{game}}"
    adb shell am force-stop $(adb shell pm list packages | grep -i "{{game}}" | head -n1 | cut -d: -f2 | tr -d '\r')

remove game="nether":
    adb uninstall $(adb shell pm list packages | grep -i "{{game}}" | head -n1 | cut -d: -f2 | tr -d '\r')

