# Nether Earth — Android TV / ARM32 build

An SDL-free Android wrapper around the existing C++ game, producing a single
`armeabi-v7a` APK for Android TV.

## Design

- **No SDL, no SDL_mixer, no vendored third-party sources.** Only the Android
  NDK and the framework are used: EGL + OpenGL ES 1.x for rendering, Android
  key/joystick events for input, and Java `SoundPool` for the sound effects.
- **ARM32 only:** `armeabi-v7a`. No other ABI is built.
- **The repository's own `assets/` tree is packaged directly.** Gradle's asset
  source directory is `../../assets`, so there is no second copy to keep in
  sync. Do **not** create `android/app/src/main/assets`.
- At runtime the assets are read from the APK with `AAssetManager`; nothing is
  extracted to internal storage. The game's `FILE*` parsers are served from a
  small in-RAM cache via `fmemopen()`, which is why `minSdk` is 23.
- `nether.cfg` and the four save-game slots are writable data and live in
  `Context.getFilesDir()`, not in the read-only APK assets.

The platform-specific code is four files in `src/`:

| File | Replaces | Role |
| --- | --- | --- |
| `platform_android.cpp` | `platform.cpp` | EGL/GLES surface, timing, key & gamepad state |
| `audio_android.cpp` | `audio.cpp` | `SoundPool` playback |
| `assets_android.cpp` | `assets.cpp` | `AAssetManager` reads, app-private writes |
| `android_bridge.cpp` | — | JNI glue and the game thread |

Nothing outside those files is Android-specific, and the desktop SDL2 build is
unaffected (`make`).

## Prerequisites

- JDK 17 or newer (`JAVA_HOME`).
- An Android SDK at `ANDROID_HOME` (or `sdk.dir` in `android/local.properties`)
  containing:

```bash
sdkmanager "platforms;android-35" "build-tools;35.0.0" \
           "ndk;27.0.12077973" "cmake;3.22.1" "platform-tools"
```

Gradle itself does not need to be installed — the wrapper (`./gradlew`)
fetches the version this project is built against.

## Build

```bash
cd android
./gradlew :app:assembleRelease      # -> app/build/outputs/apk/release/app-release.apk
```

or, from the repository root, `just apk`.

**Use the release variant on the TV.** Debug variants compile the native code
`-O0`, which is noticeably slower; the release variant is built `-O2`. Without
a key of your own, the release APK is signed with the local Android debug key,
which is enough to sideload but not to publish. To use a real key, pass (or put
in `~/.gradle/gradle.properties`):

```
NETHER_KEYSTORE=/path/to/keystore.jks
NETHER_KEYSTORE_PASSWORD=...
NETHER_KEY_ALIAS=...
NETHER_KEY_PASSWORD=...
```

## Install

```bash
adb connect <tv-ip>
adb install -r android/app/build/outputs/apk/release/app-release.apk
```

`just deploy` builds, installs and launches; `just logs` tails the game's
logcat tag (`NetherEarth`).

The app registers both a normal `LAUNCHER` and an Android TV
`LEANBACK_LAUNCHER` entry (with a banner), and declares that it needs no
touchscreen.

## Controls

Everything is playable from a gamepad or a bare TV remote's D-pad. Gamepad
input is translated into the same virtual keys the game already polls, using
whatever the player bound in *REDEFINE KEYBOARD*, so remapping keys remaps the
controller too.

| Control | Action |
| --- | --- |
| D-pad / left stick | movement (the bound up/down/left/right keys) |
| A / Y | fire (the bound fire key) |
| Start / LB / B / **BACK** | in-game menu (the bound pause key) |
| Right stick up/down, LT/RT, RB | zoom (PageUp / PageDown) |

There are no on-screen touch controls. Keys the game does not bind (volume and
so on) are passed back to the system, so the TV's own remote keeps working.

Leave the game through *EXIT GAME* in the main menu (or F12 on a keyboard);
that closes the task. HOME suspends it: the game parks itself while there is
no surface to draw on and resumes where it left off.

## Notes and limitations

- The game uses the fixed-function pipeline, so the backend targets
  `GLESv1_CM`. Linking with `-Wl,--no-undefined` against that library is what
  guarantees no desktop-only GL entry point is left in the code.
- Shadows need a stencil buffer. The EGL config asks for 8 stencil bits and
  falls back to a config without one, in which case `platform_stencil_bits()`
  reports 0 and the game turns shadows off rather than drawing them wrong.
- The GL context is deliberately *preserved* while the app is backgrounded, so
  the uploaded textures survive being suspended. If the driver reports
  `EGL_CONTEXT_LOST` anyway (a GPU reset), that is logged and the app needs a
  restart — the game has no path to re-upload its textures on demand.
- *RESOLUTION* and *FULLSCREEN* in the OPTIONS menu do nothing here: the
  SurfaceView always covers the whole screen at its native size. *SHADOWS* and
  *DETAIL* are the knobs that matter if the TV's GPU struggles.
- The Android SDK/NDK toolchain is required to build. The project intentionally
  does not fetch, vendor or clone SDL.
