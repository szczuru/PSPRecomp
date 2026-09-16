# VCSNative — Nintendo Switch bring-up starter

## Fork this repo, not the macOS one

Fork **https://github.com/jessicanataliagta/PSPRecomp** (the actively
maintained upstream — 87★, full DX12 baseline, this is where the real commit
history and issue tracker live). Skip `OverkillLabs/PSPRecomp-MacOS`: its only
addition over upstream is a Vulkan/MoltenVK + SwiftUI host layer that a Switch
build has no use for, and it would just be extra platform code to drag along.

## What's actually in this patch

Three files, taken from the real `jessicanataliagta/PSPRecomp` source tree:

- `profiles/vcs/host/display_window.cpp` — added an `#elif defined(__SWITCH__)`
  branch alongside the existing `_WIN32` branch and the existing headless
  fallback. Uses libnx `Framebuffer`/`nwindow` to blit the CPU-decoded PSP
  frame to the screen (nearest-neighbour upscale, letterboxed) and `PadState`
  for input, mapped onto the same `HostInputState` every other platform fills.
- `profiles/vcs/host/audio_output.cpp` — same pattern, using libnx `audout`
  (fixed 48 kHz stereo PCM) with a linear resampler and a 4-buffer queue.
- `profiles/vcs/CMakeLists.txt` — added a `NINTENDO_SWITCH` branch next to the
  existing `WIN32`/generic branches: links `switch-ffmpeg` from devkitPro's
  portlibs instead of the bundled Windows `.lib`s, links `nx`, and packages
  the result as `.nro` via `nx_generate_nacp`/`nx_create_nro` **if** you
  configure with devkitPro's own toolchain file (see below).

`.diff` files for all three are included too, in case you'd rather apply them
by hand or review them line-by-line instead of copying the full files.

## Why so little needed to change

`ge_gpu_backend_dx12.cpp` (the GE/GPU backend) is *already* fully guarded
behind `#if defined(_WIN32)`, with a working non-Windows stub at the bottom
that reports `GeGpuBackendKind::Software` and lets the CPU rasterizer own the
whole frame. `main.cpp`'s executable-directory resolution already has a
portable non-Windows branch too. So this is not a new profile and not a
rewrite — it's the same three-branch pattern (`_WIN32` / `__SWITCH__` /
generic headless) the project already uses, extended to a platform that isn't
headless.

## What this is NOT

- **No GPU acceleration.** Everything still runs on the existing bit-exact
  CPU rasterizer; the Switch code only presents its output. That's the
  correct order of operations (matches how the project itself brought up
  Vulkan/DX12 — Software first, GPU backend later as its own stage), but
  don't expect DX12-parity performance out of this.
- **No romfs/SD-card asset wiring.** `vcs_bootstrap_paths.cpp` still looks for
  `PSP_DATA` next to the executable; on Switch that means next to the `.nro`
  on the SD card, or inside a `romfs` you build in yourself. Not done here.
- **Untested.** I don't have a devkitA64 toolchain in this environment, so
  none of this has been compiled. Treat it as a correct-on-paper starting
  point, not a working build — expect at least a few libnx API-signature
  fixes on the first real compile.
- **Audio volume is dropped.** `audio_output_submit`'s `left`/`right`
  parameters are almost certainly per-channel PSP volume, not something my
  placeholder mixer uses. Worth porting the real logic from
  `audio_resampler.hpp` once you can read it against actual behavior on
  another platform.

## Building on GitHub Actions

`.github/workflows/switch-build.yml` builds `VCSNative` for Switch on every
push/PR to a `switch-port` branch (or manually via "Run workflow"), using the
official `devkitpro/devkita64` container image so you don't need a local
toolchain at all. It installs `switch-ffmpeg` (not in the base image, needed
by the `NINTENDO_SWITCH` branch in `CMakeLists.txt`), configures with
devkitPro's own `Switch.cmake` toolchain file, builds just the `VCSNative`
target, and uploads whatever `.nro`/`.nacp` it produces as a workflow
artifact. Same caveat as everything else here: written against the
documented image/package names, not run — the first CI run is likely to
surface a package-name or path fix before it goes green.

## Building locally

1. Fork/clone `jessicanataliagta/PSPRecomp`, apply this patch.
2. Install devkitPro + devkitA64 + `switch-tools`, `switch-ffmpeg`,
   `switch-libnx` via `dkp-pacman`.
3. Configure with devkitPro's own CMake toolchain, e.g.:
   ```
   cmake -B build-switch -G Ninja \
     -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake \
     -DPSPRECOMP_PROFILE=vcs
   cmake --build build-switch --target VCSNative
   ```
   That toolchain file is what defines `NINTENDO_SWITCH` and `__SWITCH__` for
   the branches above — I intentionally didn't write my own, since devkitPro
   already ships and maintains one.
4. First build will surface the real libnx signatures where my recollection
   is off — fix those before worrying about anything else.


====
org readme:
# PSPRecomp

PSPRecomp is a static recompilation framework for PSP software. It reads an Allegrex/MIPS executable, analyzes guest code, emits C++ translation units, and runs them through a native host runtime instead of shipping a PSP interpreter or JIT.

The repository is split between a reusable framework and game-specific profiles. The first working profile is GTA: Vice City Stories (`profiles/vcs`).

## Repository layout

```text
include/psprecomp/   Public runtime and Allegrex interfaces
src/                 ELF/PRX loading, decoder, memory, runtime and support code
tools/               Generic analyzer, recompiler and reverse-engineering helpers
tests/               Framework regression tests
configs/             Generic examples and PSP NID data
profiles/            Game-specific hosts, generated code, configuration and tests
  vcs/               GTA: Vice City Stories profile
```

Game-specific addresses, HLE behavior, native fast paths, renderer integration and generated AOT code belong under a profile. The framework should remain usable without any profile selected.

## Requirements

- CMake 3.20 or newer
- A C++20 compiler
- Visual Studio 2022 for the current Windows/DX12 VCS build

## Build the framework only

```bash
cmake -S . -B out/framework -DPSPRECOMP_PROFILE=""
cmake --build out/framework --config Release
ctest --test-dir out/framework -C Release --output-on-failure
```

This builds `psprecomp_core`, `psp_analyze`, `psp_recomp`, `dump_function` and the framework tests.

## Build a profile

Profiles are selected with `PSPRECOMP_PROFILE`:

```bash
cmake -S . -B out/vcs -DPSPRECOMP_PROFILE=vcs
cmake --build out/vcs --config Release
```

Windows users working on the VCS profile can use the maintained scripts in `profiles/vcs/scripts`.

## Create another profile

See [`docs/PROFILE_GUIDE.md`](docs/PROFILE_GUIDE.md). A new title normally provides its own generated corpus, HLE/profile host, configuration, tests and optional native fast paths without modifying the framework for game-specific addresses.

## Game files

No commercial game executable or asset is included. PSPRecomp does not ship an EBOOT decryption implementation. Profiles expect files obtained from the user's own copy in the format documented by that profile.

## Source provenance

See [`docs/SOURCE_PROVENANCE.md`](docs/SOURCE_PROVENANCE.md) for the project rules around independently written code, profile boundaries, decryption and third-party source.

## Third-party code

The framework is MIT licensed. Individual profiles may include separately licensed dependencies or assets; their notices stay beside those files. The VCS profile lists its bundled dependencies in `profiles/vcs/THIRD_PARTY.md`.

## License

PSPRecomp framework code is distributed under the MIT License. See [`LICENSE`](LICENSE).
