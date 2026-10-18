# YOUR HALO

Windows VST3 vocal enhancer with a single amount control and four tone modes for brightness, density, and stereo character.

## Overview

A Windows x64 VST3 vocal enhancer built with JUCE 8.0.12 and C++17. The HALO orb controls the amount from 0–100%; the existing AURA, GLOW, SHINE and ASCEND selectors choose balanced, warm, bright or dramatic processing character.

## Features

- Set enhancement amount with the HALO orb.
- Choose AURA, GLOW, SHINE, or ASCEND processing character.
- Combine tone shaping, compression, saturation, and presence enhancement.
- Enhance existing stereo side content while supporting matched mono/stereo layouts.
- Automate amount and mode and restore them with DAW sessions.
- View audio-reactive artwork with zero reported plugin latency.

## Screenshots

![YOUR HALO vocal enhancer](Docs/Screenshots/Screenshot%202026-10-17%20000439.png)

## Technologies

C++17, JUCE 8.0.12, VST3, CMake, MSVC, and Inno Setup.

## Getting Started

Download the [official v1.0.0 release](https://github.com/scryingorb/YOUR-HALO/releases/tag/v1.0.0):

- `YOUR-HALO-1.0.0-Windows-x64-Setup.exe`: Windows installer with uninstall support.
- `YOUR-HALO-1.0.0-Windows-x64.zip`: clean manual-install VST3 distribution.
- `SHA256SUMS.txt`: download integrity checksums.

Close your DAW. Run the installer, or extract the ZIP and copy the complete `YOUR HALO.vst3` bundle to `C:\Program Files\Common Files\VST3\VoxxLab`. Preserve its `Contents` folder. Rescan VST3 plugins in your DAW; in FL Studio, use **Options > Manage plugins > Find installed plugins**. See [Windows installation](Docs/Windows-Installation.md).

Requirements: Windows 10 or later and a 64-bit VST3 host. The plugin is not a standalone application. Installer and plugin are unsigned.

### Build or run from source

Use Visual Studio desktop C++ tools, a Windows SDK and CMake 3.22+. CMake fetches JUCE 8.0.12 if `HALO_JUCE_PATH` is omitted; an existing checkout can be supplied explicitly.

```powershell
cmake -S . -B build -A x64 -DHALO_JUCE_PATH=C:/path/to/JUCE
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release -R '^HaloTests$' --output-on-failure
```

The core suite does not require compiled reference artifacts. The additional `HaloVisualTests` historical comparison requires the retained prototype at `dist/YOUR HALO.vst3`, intentionally not tracked in Git. In a development workspace containing that reference, `ctest --test-dir build -C Release --output-on-failure` runs both suites. `build.ps1` performs the complete build/test workflow for that workspace; it supports explicit CMake and JUCE paths. Use `-DHALO_BUILD_TESTS=OFF` for a plugin-only build.

Output: `build/YOUR_HALO_artefacts/Release/VST3/YOUR HALO.vst3`.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\package.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\build-installer.ps1
```

ZIP output: `dist/releases/1.0.0`. Installer output: `dist/installer`; Inno Setup 6.3+ is required. [Installer build/test documentation](Docs/Installer.md) includes the isolated per-user smoke-test procedure. All binary outputs remain ignored; GitHub Releases carries downloadable artifacts. The retained early prototype is not overwritten.

Source is in `Source/`, required embedded artwork in `Assets/`, tests in `Tests/`, and dependency notices in `ThirdPartyLicenses/`. JUCE licensing obligations remain applicable; bundled notices do not grant a commercial JUCE entitlement. The [architecture](Docs/Architecture.md) and [DSP design report](Docs/DSPUpgrade/Report.md) describe the existing engine. Older visual-development reports are historical snapshots, not final release acceptance claims.

## Usage

Processing combines tone shaping, stereo-linked soft-knee compression and parallel density, band-limited harmonic saturation, presence and air, dynamic harshness control, existing-side enhancement, and soft sample-peak protection.

- **AURA:** balanced vocal polish.
- **GLOW:** warmer body and richer harmonics.
- **SHINE:** open air and forward presence.
- **ASCEND:** the strongest density and existing-side enhancement.

Drag or scroll the orb, use arrow keys for fine changes, and double-click to reset. Settled HALO zero is neutral, with 35 ms smoothing and zero reported latency. Both `halo` and `mode` are automatable and saved with sessions. Original plugin/HALO identifiers remain; older sessions without MODE restore AURA. Mono and stereo input/output layouts must match.

The pearl/gold interface retains the official embedded HALO artwork and continuous, audio-reactive aurora light. No external desktop artwork is needed at runtime.

## Validation and limitations

The release was tested at 44.1/48/88.2/96/176.4/192 kHz with mono/stereo, multiple buffer sizes, automation, state recall, zero/overload/silence behavior, repeated editor lifecycles and scaled UI renders. Tracktion pluginval strictness 5 passed command-line validation. Installer payload hashes, installed VST3 host tests, uninstall and extracted ZIP contents were checked. See the [release QA report](Docs/ReleaseQA/Report.md) and [CHANGELOG](CHANGELOG.md).

Native DAW GUI testing, real-vocal listening, hardware DPI transitions and default all-users/UAC installation were not performed. FL Studio and other hosts therefore remain manual acceptance checks. Pluginval GUI tests and Steinberg's separate validator were not run. Peak protection is not true-peak limiting; nonlinear processing is not oversampled. These limitations are documented rather than represented as passed tests.

## Project Status

Released: version 1.0.0 for Windows x64. Native DAW GUI, real-vocal listening, and hardware DPI acceptance checks remain pending.
