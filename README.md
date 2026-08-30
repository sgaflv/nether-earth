# Nether Earth Remake

A full 3D remake (in C++/OpenGL/SDL2) of **Nether Earth**, one of the earliest
real-time strategy games, originally released in 1987 for the ZX Spectrum,
Commodore 64 and Amstrad CPC.

This branch has been ported from the original SDL 1.2 build to **SDL2** /
**SDL2_mixer**.

## Building

Requires a C++ compiler, `make`, and the SDL2 + OpenGL development packages.

### Fedora

```bash
sudo dnf install gcc-c++ make \
  SDL2-devel SDL2_mixer-devel \
  mesa-libGL-devel mesa-libGLU-devel freeglut-devel
```

### Debian / Ubuntu

```bash
sudo apt install build-essential \
  libsdl2-dev libsdl2-mixer-dev \
  libgl1-mesa-dev libglu1-mesa-dev freeglut3-dev
```

### macOS

Install SDL2 and SDL2_mixer (e.g. via Homebrew) and freeglut, then `make`.

Then build:

```bash
make
```

The binary is `nether_earth`. Rebuild from scratch with `make clean && make`.

## Running

The game reads its config, maps, models, textures and sounds from the current
working directory, so run it from the source directory:

```bash
./nether_earth
```


Controls can be redefined from the main menu (and are saved in `nether.cfg`).
Default keys: Q/A = up/down, O/P = left/right, Space = fire, F1 = pause/menu.
The window is resizable; Alt+Enter toggles fullscreen.

## Credits and attribution

This is a remake of the original 1987 game by **Icon Design Ltd**, published by
Argus Press Software.

- **Author of the remake:** Santiago Ontañón Villar (2002)
- **Linux port:** Pavel Cejka
- **RPM packaging:** Ilya Kuznecov
- **Release packaging (0.51/0.52):** Andrej


## License

Note: **there is no license file in this repository.** The remake was released
by the original author as open-source/freeware under an informal **"as-is"
license** (commonly listed as such by third-party sources such as OSGC and
Osgameclones), and it is packaged in several Linux distributions. However,
because no written license text is included in the source tree, you should
treat the code as provided "as-is" and confirm terms with the original author
before redistributing or relicensing it.

The original 1987 game is a separate copyrighted work; this is an independent
fan remake.
