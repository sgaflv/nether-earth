# Goal: Prepare Nether Earth for Android Without Building an APK Yet

## Purpose

Prepare the existing Nether Earth source tree for a future Android build.

**This phase must NOT create an APK or an Android application project.**

The objective is to first make the existing native project:

* independent of unnecessary desktop-only dependencies;
* portable to Android;
* compatible with OpenGL ES where required;
* capable of loading its assets through an Android-compatible abstraction;
* free of unnecessary SDL dependencies;
* ready for a later, clean Android packaging phase.

Only after this preparation phase is complete should an Android/Gradle/APK project be introduced.

---

# 1. Do Not Create an Android APK Yet

Android packaging is explicitly out of scope for this phase.

Do NOT:

* create `android/app/`;
* create a Gradle project;
* create `AndroidManifest.xml`;
* add Java/Kotlin Activity classes;
* add Android NDK build files;
* copy assets into `android/app/src/main/assets/`;
* download or vendor SDL merely to satisfy an Android build;
* produce an APK.

The existing project remains the primary project.

The Android build will be a separate final phase after the native code has been prepared.

---

# 2. Preserve the Existing Project Structure

The existing repository already has:

```text
assets/
src/
Makefile
justfile
```

Keep this structure unless there is a concrete reason to change it.

There must be exactly one authoritative source copy of the game assets:

```text
assets/
```

Do not create a second source-tree copy such as:

```text
android/app/src/main/assets/
```

or:

```text
android/assets/
```

during this preparation phase.

A future Android packaging step may package the existing `assets/` directory directly.

---

# 3. Establish a Platform Abstraction Layer

The current source mixes application/game logic with SDL and OpenGL/platform APIs.

Separate these concerns before attempting Android packaging.

The desired architecture is:

```text
Game/application code
        |
        +----------------+
        |                |
   graphics          platform
   abstraction       abstraction
        |                |
        v                v
     GLES/GL        SDL / Android
```

The application/game logic should not need to know whether it is running through:

* desktop SDL;
* Android;
* another future platform.

Avoid a large rewrite.

Extract only the APIs actually required by the project.

---

# 4. SDL Audit — First Priority

Before deleting SDL, identify every SDL dependency.

Search the entire source tree for:

```text
SDL_
SDL.h
SDL_mixer
SDL_mixer.h
```

Classify every use.

Current known SDL functionality includes at least:

* `SDL_Init`
* `SDL_Quit`
* `SDL_Window`
* `SDL_GLContext`
* `SDL_CreateWindow`
* `SDL_GL_CreateContext`
* `SDL_GL_DeleteContext`
* `SDL_GL_SetAttribute`
* `SDL_GL_SwapWindow`
* `SDL_GL_GetDrawableSize`
* `SDL_GL_MakeCurrent`
* `SDL_PumpEvents`
* `SDL_GetKeyboardState`
* `SDL_ShowCursor`
* `SDL_GetTicks`
* `SDL_SCANCODE_*`
* SDL audio through `SDL_mixer`
* `Mix_Chunk`
* `Mix_PlayChannel`
* `Mix_Volume`

Produce a dependency table before removing anything.

Example:

| SDL API                | Current use | Replacement candidate      |
| ---------------------- | ----------- | -------------------------- |
| `SDL_GetTicks`         | Timing      | monotonic clock            |
| `SDL_SCANCODE_*`       | Keyboard    | platform input abstraction |
| `SDL_GetKeyboardState` | Input       | platform input abstraction |
| `SDL_Window`           | Window      | platform abstraction       |
| `SDL_GL_*`             | GL context  | platform/EGL layer         |
| `Mix_Chunk`            | Audio       | audio abstraction          |
| `Mix_PlayChannel`      | SFX         | audio abstraction          |

Do not assume SDL is required merely because it is currently included.

---

# 5. Determine Whether SDL Can Be Removed Completely

The preferred outcome is:

```text
NO SDL DEPENDENCY
```

if the actual application requirements allow it.

Do not add 2,000+ SDL source files merely because Android is the target.

Android does not provide a system `libSDL`.

Therefore:

```text
Android system
    !=
system SDL library
```

If SDL is removed, replace its functionality with small project-owned interfaces.

If removing SDL would require rewriting substantial game logic, retain only the minimum necessary abstraction or consider a much smaller SDL integration later.

The decision must be based on the actual dependency audit.

---

# 6. SDL Mixer Must Be Audited Separately

`SDL_mixer` is a distinct dependency and must not automatically survive merely because SDL itself is retained.

Determine:

* number of sound effects;
* formats used;
* whether only WAV files are used;
* whether music is used;
* simultaneous channel requirements;
* volume/panning requirements;
* looping requirements.

If the project only requires simple WAV sound effects, investigate replacing SDL_mixer with a much smaller native/platform audio implementation.

Do not add another large audio dependency without evidence that it is necessary.

---

# 7. Replace SDL Timing

The current code uses:

```cpp
SDL_GetTicks()
```

Identify every timing use.

Introduce a project-owned timing function, for example conceptually:

```cpp
uint32_t platform_ticks();
```

or an equivalent abstraction.

Desktop implementation can use a monotonic clock.

The future Android implementation can use an Android-compatible monotonic clock.

Game logic must not directly depend on SDL timing.

---

# 8. Replace SDL Keyboard Constants

The game currently stores:

```cpp
unsigned char old_keyboard[SDL_NUM_SCANCODES];
```

and directly tests values such as:

```cpp
SDL_SCANCODE_Q
SDL_SCANCODE_A
SDL_SCANCODE_O
SDL_SCANCODE_P
SDL_SCANCODE_SPACE
SDL_SCANCODE_F1
SDL_SCANCODE_PAGEUP
SDL_SCANCODE_PAGEDOWN
SDL_SCANCODE_KP_MINUS
SDL_SCANCODE_KP_PLUS
```

Create a project-owned input representation.

For example:

```text
KEY_Q
KEY_A
KEY_O
KEY_P
KEY_SPACE
KEY_F1
...
```

The platform layer translates physical/platform events into this representation.

Do not expose SDL scancode values throughout the game.

---

# 9. Replace SDL Event Polling

The current code uses:

```cpp
SDL_PumpEvents();
SDL_GetKeyboardState(NULL);
```

Replace this with an application-owned input state.

Conceptually:

```text
platform_input_update()
        |
        v
application keyboard/input state
        |
        v
game logic
```

This makes the game independent of SDL.

The Android implementation can later populate the same state from Android touch, keyboard, controller, or other input events.

---

# 10. Remove SDL Window Ownership From Game Logic

The current `main.cpp` owns:

```cpp
SDL_Window *window;
SDL_GLContext glcontext;
```

and creates the OpenGL context itself.

Move this responsibility into a platform/rendering boundary.

The game should eventually receive:

```text
initialized rendering environment
```

rather than creating an SDL window itself.

The future implementations can then be:

```text
Desktop:
SDL → OpenGL

Android:
Java Activity → Surface/EGL → GLES
```

Do not implement the Android side yet.

---

# 11. OpenGL Compatibility Audit

The project is currently heavily dependent on the fixed-function OpenGL API.

Known examples include:

```cpp
glBegin
glEnd
glMatrixMode
glPushMatrix
glPopMatrix
glTranslatef
glRotatef
glScalef
glColor3f
glColor4f
glLightfv
glEnable(GL_LIGHTING)
glEnable(GL_COLOR_MATERIAL)
glEnableClientState
glVertexPointer
glNormalPointer
```

This is a major Android/GLES concern.

Do not attempt to solve everything by simply changing:

```cpp
#include <GL/gl.h>
```

to:

```cpp
#include <GLES/gl.h>
```

That will not make desktop OpenGL code compatible.

---

# 12. Decide the GLES Target

Determine whether the renderer should target:

```text
OpenGL ES 1.x
```

or:

```text
OpenGL ES 2.0+
```

The existing renderer appears intentionally close to an OpenGL ES 1-style API in several places.

Evidence includes the existing:

```text
src/glport.h
src/glport.cpp
```

and comments concerning GLES1 compatibility.

However, the final decision must be based on the complete renderer audit.

Prefer the lowest GLES version that allows the existing renderer to work correctly without excessive emulation.

---

# 13. Use `glport` as the Starting Point

Inspect and expand the existing:

```text
src/glport.h
src/glport.cpp
```

rather than creating an unrelated rendering abstraction.

The existing code already provides compatibility functionality such as:

```text
glOrtho(...)
glColor3f(...)
```

where GLES1 lacks the desktop equivalent.

Determine whether `glport` is intended to become the project's GL compatibility layer.

If appropriate, centralize compatibility wrappers there.

Do not scatter Android-specific GLES compatibility hacks throughout gameplay code.

---

# 14. Fixed-Function Rendering Audit

Identify all uses of:

```text
glBegin/glEnd
glMatrixMode
glPushMatrix/glPopMatrix
glTranslate*
glRotate*
glScale*
glColor*
glLight*
glMaterial*
glTexEnv*
glEnableClientState
glVertexPointer
glNormalPointer
```

For each call, classify:

1. directly available in target GLES;
2. available with a small wrapper;
3. requires renderer emulation;
4. requires conversion to a shader-based implementation;
5. unused/dead code and removable.

Do not rewrite working rendering code unless necessary.

---

# 15. Texture Upload Audit

The current code contains texture creation such as:

```cpp
glTexImage2D(...)
```

and uses:

```text
GL_RGBA
GL_UNSIGNED_BYTE
```

Audit:

* source bitmap formats;
* dimensions;
* power-of-two requirements;
* alpha;
* texture filtering;
* texture wrapping;
* mipmaps;
* texture memory consumption;
* internal format assumptions.

There is at least one suspicious use:

```cpp
glPixelStorei(GL_UNPACK_ALIGNMENT, tname);
```

Verify whether this is actually intended.

It appears likely that the texture name is accidentally being passed as the unpack alignment.

Correct it only after confirming the intended behavior.

---

# 16. Asset Format Audit

Do not assume all existing assets should remain in their current format.

Inventory:

```text
assets/maps/*.map
assets/models/*.ase
assets/models/*.asc
assets/sound/*
assets/textures/*
```

For each format determine:

* whether it is parsed at runtime;
* whether Android can access it efficiently;
* whether conversion would reduce startup time;
* whether conversion would reduce APK size;
* whether conversion would simplify the renderer;
* whether the existing loader is robust enough.

Do not convert assets merely because they are old.

---

# 17. Consider Converting Runtime Assets

Asset conversion is allowed and should be considered where it provides a concrete benefit.

Potential candidates include:

### Models

Current formats include:

```text
ASE
ASC
```

Determine whether these are custom/legacy 3D Studio formats and whether they are parsed at startup.

If the parser is fragile or unnecessarily expensive, consider converting models into a simple project-owned binary format.

Possible pipeline:

```text
ASE/ASC
   |
   | offline conversion
   v
project model format
   |
   v
fast runtime loader
```

Keep the original source assets if they are useful as authoring/source material.

Generated runtime assets should be reproducible by a build script.

---

# 18. Texture Conversion

Inventory texture files and their formats.

Determine whether converting them to a more suitable runtime representation is beneficial.

Possible goals:

* smaller storage;
* faster loading;
* predictable RGBA format;
* removal of unnecessary legacy BMP parsing;
* better Android/GLES compatibility.

Do not introduce GPU-specific compressed texture formats unless there is a clear reason.

Because the project is intended for one ABI but potentially many Android GPU implementations, prefer broadly supported formats unless APK size/performance makes compression worthwhile.

---

# 19. Sound Conversion

Inventory the sound files.

Determine whether they are all WAV and whether the application uses only short sound effects.

If so, consider:

* normalized PCM format;
* a compact runtime representation;
* pre-conversion during build;
* replacing SDL_mixer with a minimal audio abstraction.

Do not convert audio destructively without preserving the originals.

---

# 20. Asset Path Abstraction

The current code directly references paths such as:

```text
assets/maps/
assets/models/
assets/textures/
```

and uses standard filesystem APIs.

Examples include:

```cpp
fopen(...)
opendir(...)
FindFirstFile(...)
```

This is not automatically compatible with Android packaged assets.

Introduce an asset abstraction before Android packaging.

Conceptually:

```text
asset_open("maps/city.map")
asset_list("maps")
asset_exists(...)
```

The desktop implementation can continue to use:

```text
assets/
```

The future Android implementation can access packaged assets.

The game should not know where the assets physically reside.

---

# 21. Remove Windows-Specific Asset Enumeration

The source contains code using:

```cpp
FindFirstFile(...)
```

alongside:

```cpp
opendir(...)
```

Audit and eliminate platform-specific directory enumeration from game logic.

Use the new asset abstraction.

This is especially important because Android will not provide the Windows API.

---

# 22. Fix File Access Modes

Audit all:

```cpp
fopen(..., "r+")
```

and similar modes.

The ASE loader currently opens model files with:

```cpp
fopen(file,"r+")
```

If the file is only being read, change the loader to read-only access:

```text
"r"
```

where safe.

This avoids unnecessary write requirements and is more appropriate for packaged/read-only assets.

---

# 23. Runtime-Generated Files

Identify files written by the game, including:

```text
save games
debug reports
configuration
temporary files
```

Separate them from read-only assets.

The future Android implementation will need a writable application-data location.

Do not attempt to write generated data into:

```text
assets/
```

---

# 24. GL Error and Capability Diagnostics

Add a lightweight graphics diagnostics facility.

At initialization, record:

* GL/GLES version;
* renderer;
* vendor;
* supported extensions where relevant.

During development, report:

* shader errors if shaders are introduced;
* invalid GL operations;
* framebuffer failures;
* texture failures.

Do not leave excessive logging enabled in the final release build.

---

# 25. Remove Dead Desktop Dependencies

After introducing abstractions, remove includes and dependencies that are no longer needed.

Examples to investigate:

```text
windows.h
desktop OpenGL headers
SDL.h
SDL_mixer.h
platform-specific filesystem APIs
desktop GL libraries
```

Do not delete a dependency until the source and build are verified without it.

---

# 26. Desktop Build Must Remain Working During Preparation

The preparation phase should preserve a working desktop build whenever practical.

The desired progression is:

```text
Current desktop project
        |
        v
platform-independent game code
        |
        v
GLES-compatible renderer
        |
        v
portable asset system
        |
        v
portable audio/input/timing
        |
        v
clean native project
        |
        +---- desktop backend
        |
        +---- future Android backend
```

Do not make Android changes that prevent testing the game on desktop unless there is no reasonable alternative.

---

# 27. Build-System Cleanup

The current Makefile contains:

```text
pkg-config --cflags sdl2 SDL2_mixer
pkg-config --libs sdl2 SDL2_mixer
-lGL
```

These are desktop-specific dependencies.

Do not immediately replace them with Android NDK configuration.

Instead, first introduce a clean platform dependency model.

For example:

```text
common sources
    +
desktop platform sources
    +
desktop libraries
```

Later:

```text
common sources
    +
Android platform sources
    +
Android system libraries
```

The Android build system will be introduced only after this phase.

---

# 28. No Vendored SDL Unless Proven Necessary

Do not add a large SDL source tree to the repository merely to make future Android compilation easier.

Before any SDL source is added:

1. Complete the SDL API audit.
2. Determine which SDL subsystems are actually required.
3. Determine whether those subsystems can be replaced by small project-owned abstractions.
4. Determine whether Android provides a suitable native replacement.
5. Measure the resulting complexity.

The preferred dependency order is:

```text
Existing code
    ↓
C/C++ standard library / POSIX
    ↓
Android native APIs where necessary
    ↓
small project-owned wrappers
    ↓
SDL only if genuinely useful
```

SDL is not an Android system dependency and must not be treated as one.

---

# 29. No Asset Duplication

There must be no manually maintained duplicate asset tree.

Bad:

```text
assets/
android/app/src/main/assets/
```

Good:

```text
assets/
```

with a future Android build rule that packages this directory directly.

If generated runtime assets are introduced, make them reproducible:

```text
assets-source/
        |
        | converter
        v
generated-assets/
```

If this approach is used, document exactly which directory is authoritative and ensure generated files are not manually edited.

---

# 30. Testing Strategy

Before APK work begins, establish a native testable state.

At minimum verify:

### Build

* [x] Native project builds.
* [x] No unnecessary SDL dependency remains.
* [x] No accidental desktop-only dependency remains in common code.

### Renderer

* [x] Existing renderer initializes.
* [x] All GL calls are accounted for.
* [x] GLES incompatibilities are documented/resolved.
* [x] Texture loading works.
* [x] Model rendering works.
* [x] Lighting works.
* [x] Transparency works.
* [x] Shadows work.
* [x] 2D/menu rendering works.

### Assets

* [x] All maps load.
* [x] All required models load.
* [x] All required textures load.
* [x] All sounds load (format-audited; runtime audio needs a device).
* [x] Asset paths are centralized.

### Game

* [ ] Main menu works.
* [ ] Construction mode works.
* [ ] Gameplay works.
* [ ] AI works.
* [ ] Save/load works.
* [ ] Sound effects work.
* [x] Debug/report functionality is understood.

---

# 31. Explicitly Track Known Porting Problems

Maintain a section in this file or a separate porting checklist containing discovered incompatibilities.

Initial known areas include:

```text
SDL dependency
SDL_mixer dependency
desktop OpenGL fixed-function API
OpenGL matrix stack
OpenGL lighting
OpenGL client arrays
desktop glOrtho/glColor compatibility
filesystem/path handling
Windows FindFirstFile
SDL keyboard state
SDL timing
SDL window/context creation
SDL buffer swap
audio backend
asset packaging
read/write file assumptions
```

Every item must eventually have one of:

```text
KEEP
REPLACE
WRAP
REMOVE
CONVERT
DEFER
```

Status of each known area (as of the SDL/asset/GLES audit):

```text
SDL dependency                  WRAP    - confined to platform.cpp (window/input/GL context/swapping)
SDL_mixer dependency            WRAP    - confined to audio.cpp; only WAV SFX used, no music
desktop OpenGL fixed-function API  CONVERT - GLES1 is the target (see #12); glOrtho/glColor3f wrapped in glport
OpenGL matrix stack             KEEP    - GLES1 matrix stack; no client-side matrix code needed
OpenGL lighting                 KEEP    - GL_LIGHT0/glLightfv only, all present in GLES1
OpenGL client arrays            KEEP    - glVertex/Normal/TexCoord/ColorPointer + glDrawArrays are GLES1 API
desktop glOrtho/glColor compatibility  WRAP - glOrtho (GLdouble) + glColor3f wrappers in glport.cpp (ES-only)
filesystem/path handling        WRAP    - assets.cpp asset_open/asset_list + user_open (accessed via assets.h)
Windows FindFirstFile           REMOVE  - map enumeration now uses asset_list() (POSIX opendir in the desktop backend)
SDL keyboard state              WRAP    - platform_pump_input + flat KEY_* array in platform.h
SDL timing                      WRAP    - platform_ticks()/platform_sleep()
SDL window/context creation     WRAP    - platform_create_window()/swap_buffers()/get_drawable_size()
SDL buffer swap                 WRAP    - platform_swap_buffers()
audio backend                   WRAP    - audio.h opaque Sound; audio_play(vol 0..128); native mixer replaceable per-platform
asset packaging                 DEFER   - assets/ stays authoritative; Android packaging decided later
read/write file assumptions     WRAP    - fopen(read) -> asset_open; fopen(write) -> user_open (saves/reports)
```

---

# 31a. Asset Format Audit Results (Completed)

Inventory: `assets/` total ~2.9 MB.

```text
assets/maps/      16 .map   ~204 KB
assets/models/    59 (40 .ase + 19 .asc)  ~2.5 MB
assets/textures/  34 .bmp   ~152 KB
assets/sound/      5 .wav   ~84 KB
```

## Textures (34 BMP)

* All are uncompressed 24-bit BMP (BGR, bottom-up), BI_RGB, no alpha.
* Sizes: 30 files 32x32, 1 file 64x64 (nuclear.bmp), 3 files 32x33
  (grass1/2/3.bmp), and 4 files 33x32 (lowwall*w1/w2). `createTexture`
  crops to `min(dx,dy)` so every upload is square power-of-two
  (32x32 or 64x64); `GL_UNPACK_ALIGNMENT=1` + `GL_RGBA` upload confirmed
  (`myglutaux.cpp` 648-655, 687-694). Alpha is always forced to 255; no
  alpha/transparency textures exist, so GLES blending needs are limited to
  particle/depth-write cases.
* GLES1 needs NO texture conversion: GL_RGBA + GL_UNSIGNED_BYTE + NPOT-free
  square dimensions are already GLES1-compatible.
* The BMP parser is legacy/fragile (reads width/height from fixed byte
  offsets, assumes 24-bit, ignores the pixel-data offset). It works for the
  actual asset set; a future converter to RGBA (e.g. TGA/PKM or raw RGBA)
  is a build-time option, NOT required.
* Runtime-verified: all 34/34 BMPs upload through createTexture under an
  offscreen GL context (see Models section below).

## Models (59 files: 40 ASE, 19 ASC)

* Formats are 3D Studio ASCII exports: `.ase` (with materials/textures/UVs,
  used by textured models) and `.asc` (vertex/face text, used by untextured
  models). Both are parsed at startup by `C3DObject::loadASE` /
  `C3DObject::loadASC` (3dobject-ase.cpp / 3dobject.cpp).
* Both loaders use `asset_open(file,"r")` — read-only, no write permission
  needed once applied (goal #22 satisfied).
* ASE files reference Windows absolute bitmap paths
  (`C:\Brain\Strategy\textures\*.bmp`); the loader strips the directory
  and prepends `texturedir` (`assets/textures/`). This is portable (no
  Windows API used) and pairs each model face with a texture by name.
* Texture references `grass.bmp` and `sand.bmp` appear in e-bipod.ase,
  h-bipod.ase, grass.ase, sand1.ase and do NOT exist in assets/textures/.
  Those faces bind texture 0 (render as untextured default). This is a
  pre-existing data gap, not a portability bug; fix by adding the two BMPs
  or mapping the faces to an existing texture.
* Rendering expands faces to plain triangle arrays + glDrawArrays
  (3dobject.cpp build_draw_buffers / draw); GLES1-compatible (audited
  in #14).
* Runtime-verified by a headless harness (SDL offscreen + llvmpipe GL):
  all 59/59 models load, 59/59 draw buffers build, and all 34/34 textures
  upload via createTexture; representative Piece3DObject (h-bipod) and
  Shadow3DObject (ship) also load and build buffers.
* Missing grass.bmp/sand.bmp faces bind texture 0 and render untextured —
  verified graceful (no crash), consistent with the data gap above.
* No conversion needed: ASE/ASC are small (2.5 MB total) and load quickly.
  Keep as authoritative source assets.

## Maps (16 .map)

* Simple ASCII: header `W\nH\n` (width and height on separate lines), then
  `W*H` tile tokens (G,S,S2,M,H1..H6,GG,SS,MM), then an entity section of
  fence/wallN/factory warbase placements with coordinates. Read via
  `asset_open` in `NETHER::loadmap` (maps.cpp).
* Runtime-verified by a headless harness mirroring loadmap's parse rules:
  all 16/16 maps parse (grid tokens valid, entity coordinates well-formed).
* Note: rectangle.map / rectangle2.map declare `32x64` but contain 66 grid
  rows; loadmap reads the first 2048 tokens as the grid and silently skips
  the extra 64 `G` tokens in the entity loop, so they load correctly with
  2 unused rows. Harmless data quirk, not a loader bug.
* No conversion needed.

## Sounds (5 WAV)

* All PCM WAV, mono, 16-bit, short effects only. No music/loops.
  Sample rates: 8000 (shot), 8012 (explosion), 11025 (construction,
  select, wrong). 18-19 KB each.
* `audio.cpp` opens SDL_mixer at 22050 Hz mono; mixer resamples on load.
* GLES/Android concern: SDL_mixer is desktop-only; the audio.h abstraction
  hides it. Android backend can implement a tiny 16-bit PCM mixer for these
  5 clips — no MP3/OGG/music formats exist to port.
* Conversion not required; keep the WAV originals.

## File access (recap)

* All asset reads: `asset_open()` (assets.cpp) — `assets/` prefix tolerated.
* All runtime/user files (save slots `savedgameN.txt`, `nether.cfg`,
  `report.txt`): `user_open()`.
* Map enumeration: `asset_list("maps",".map")` — no FindFirstFile/opendir
  left in game logic.

---

# 32. Definition of Done for This Phase

This preparation phase is complete when:

* [x] No Android project has been created yet.
* [x] No APK is produced yet.
* [x] SDL usage has been completely audited.
* [x] A deliberate decision has been made whether SDL is needed.
* [x] SDL has been removed if it is unnecessary.
* [x] SDL_mixer has been independently evaluated.
* [x] Platform functionality has been isolated.
* [x] Timing is independent of SDL.
* [x] Input is independent of SDL.
* [x] Window/context creation is isolated.
* [x] Asset access is abstracted.
* [x] Windows-specific filesystem code is removed from common game logic.
* [x] Runtime-generated files are separated from packaged assets.
* [x] OpenGL usage has been completely audited for GLES.
* [x] `glport` has been evaluated and reused where appropriate.
* [x] Fixed-function rendering incompatibilities have been resolved or explicitly planned.
* [x] Texture formats have been audited.
* [x] Model formats have been audited.
* [x] Sound formats have been audited.
* [x] Asset conversion opportunities have been evaluated.
* [x] The original assets remain preserved.
* [x] No manually duplicated asset tree exists.
* [x] The native project remains buildable/testable.
* [x] All remaining external dependencies are documented.
* [x] The codebase has a clear boundary between portable game code and platform code.

Only after all of the above is complete should the project proceed to:

```text
Android backend
        ↓
Android build system
        ↓
armeabi-v7a native build
        ↓
APK packaging
```

The APK phase is intentionally a separate goal.
