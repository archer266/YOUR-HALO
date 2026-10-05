# YOUR HALO · VoxxLab

A Windows x64 JUCE 8 / C++17 VST3 vocal enhancer with an automatable HALO macro and four processing modes, a 500 × 600 custom interface and zero reported latency.

**Current source: four-mode DSP upgrade with a 60 FPS fluid aurora orb.** The original 1.0.0 package remains in `dist`; the updated orb VST3 is in `build/YOUR_HALO_artefacts/Release/VST3/YOUR HALO.vst3`. No new final release has been packaged and the plugin version/identifiers remain 1.0.0. See the [DSP and visual upgrade report](Docs/DSPUpgrade/Report.md) and [captured interactive preview](Docs/DSPUpgrade/Preview.html).

Presence → air → harmonic glow → dynamic harshness protection → high-passed stereo side enhancement → level compensation and peak safety. HALO starts at zero, with exact dry output at the settled endpoint. No FFT, lookahead or delay buffer is used.

## Use the build

The current development bundle is `build/YOUR_HALO_artefacts/Release/VST3/YOUR HALO.vst3`. The original public 1.0.0 bundle and ZIP remain in `dist`. Copy the entire bundle, preserving its `Contents` folder, to `C:\Program Files\Common Files\VST3\VoxxLab\YOUR HALO.vst3`, then scan verified plugins in FL Studio. See [installation and DAW acceptance steps](Docs/FL-Studio.md).

Drag HALO, use the mouse wheel or arrow keys, and double-click to reset. The host saves and automates the `halo` parameter (0–100%, default 0). Input and output channel layouts must match: mono or stereo.

## Build and test

Install Visual Studio Build Tools with desktop C++, a Windows SDK and CMake 3.22+. JUCE is pinned to 8.0.12. Use an existing JUCE source checkout to build offline, or omit the path to fetch that version from GitHub.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -CMakePath 'C:\path\to\cmake.exe' -JucePath 'C:\path\to\JUCE'
# Packaging is deferred during DSP and visual development.
```

On this workstation, the build script can reuse the configured CMake executable and JUCE dependency from `build/CMakeCache.txt`; omit the path arguments to reuse them. The existing dependency is read only; compilation outputs belong to this project. On a fresh checkout, provide the tools as above or put CMake on PATH. The commands use a process-scoped execution policy for these local scripts and do not change the machine's policy.

```powershell
cmake -S . -B build -A x64 -DHALO_JUCE_PATH=C:/path/to/JUCE
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

Tests load the built VST3, so the VST3 target is a dependency of the test executable. Set `-DHALO_BUILD_TESTS=OFF` for a plugin-only build. The MSVC runtime is linked statically.

`HaloVisualTests` checks compatibility against the retained `dist/YOUR HALO.vst3` release and the previous development build, confirming the intentional audio change and the exact dry endpoint. The saved 1.0.0 state fixture lives in `Tests/Fixtures`. Both CTest suites write their measurements, A/B WAVs, screenshots and animation frames into `Docs/DSPUpgrade`. Open [the local review](Docs/DSPUpgrade/Review.html) for mode screenshots and synthetic listening examples.

## Source layout

- `Source/DSP`: allocation-free, sample-by-sample engine and minimum-phase filters.
- `Source/PluginProcessor.*`: host buses, HALO APVTS, state and audio lifecycle.
- `Source/PluginEditor.*`: branding and component placement.
- `Source/Metering`: lock-free input RMS/peak observer, separate from DSP.
- `Source/UI`: parameter-attached vector orb, lighting caches and animation model.
- `Assets`: original official HALO artwork, embedded through JUCE BinaryData; no runtime image-file dependency.
- `Tests`: DSP, automation, state, GUI lifecycle, memory and actual VST3 host regression tests.
- `Docs`: [architecture and HALO mapping](Docs/Architecture.md), [verification](Docs/Verification.md), UI snapshots and FL Studio instructions.

## Validation limits

The Release x64 VST3 builds and passes automated processing and host-loading tests. FL Studio interactive scanning, listening on recorded vocals, automation recording, high-DPI interaction and `.flp` save/reopen are still manual acceptance tasks. See the verification report for measurements and limitations.

Framework notices are preserved in `ThirdPartyLicenses`. JUCE's source license is included; this project does not choose a commercial JUCE plan on behalf of VoxxLab.
