**Current development validation:** See [the four-mode DSP and fluid-light report](DSPUpgrade/Report.md) and [CTest output](DSPUpgrade/CTest-output.log). The results below describe the retained original 1.0.0 release and earlier visual passes; the new sound intentionally differs.

# Build and verification · 4 October 2026

This report records the original 1.0.0 release retained in `dist`. The current source/build includes the later orb visual-development upgrade; see [its separate report](VisualDevelopment/Report.md). The package hashes below apply to the original release, not the updated build folder.

## Build

Windows x64 Release, Visual Studio 2019 Build Tools / MSVC 19.29, Windows SDK 10.0.19041, JUCE 8.0.12 and CMake. The MSVC runtime is static. All seven phases compiled successfully; the endpoint refinement and final regression additions also compiled. Logs are retained at the project root (`phase1-…` through `phase7-…`, `refinement-…`, `final-build.log`, `final-tests.log`). A CMake regeneration emitted an environment-variable warning in phase 6; the final build completed without compiler warnings or errors.

Built bundle: `build/YOUR_HALO_artefacts/Release/VST3/YOUR HALO.vst3`.

Packaged bundle: `dist/YOUR HALO.vst3`.

Binary inside either bundle: `Contents/x86_64-win/YOUR HALO.vst3`.

The package binary matches the tested build byte for byte. ZIP CRC integrity and archived binary content passed; the PE machine field is `0x8664` (x64). SHA256: `F96AC1BD05F0DA11D0990860FD9C0ADFE62FB6483355CCA8BDF9EECB3FFFF9EC`.

## Automated results

CTest: **1/1 suite passed**, with 40,401,140 sample/property assertions, in 24.67 seconds. The executable uses explicit checks in Release; assertions are not compiled away. The detailed log is preserved as `Docs/Test-output.log`.

| Check | Result |
| --- | --- |
| HALO 0/25/50/75/100%, mono/stereo, 44.1/48/88.2/96 kHz | All 40 combinations passed |
| Buffers 1/17/64/127/256/512, changed during playback | Passed |
| Settled 0% transparency, including return from processing | Bit-exact for normal finite input |
| Normal synthetic vocal gain change | Largest measured RMS difference: 0.0259171 dB |
| Normal synthetic vocal peak | 0.364979, no clipping |
| Silence and long post-overload decay | Passed at all rates/layouts; no sustained output or denormals |
| Loud input, including over 0 dBFS | Active output stayed within the full-HALO 0.99 safety ceiling |
| Stereo with identical L/R | Channels stayed identical and matched mono processing |
| Antisymmetric stereo signal | Mono sum remained cancelled |
| Fixed signal with different block partitions | Bit-identical output |
| Rapid automation on 220 Hz test tone | Largest extra sample step: 0.000129089 |
| Bright-signal return to zero | Endpoint continuity passed with active compensation/protection |
| Dynamic HF protection | Strong 7 kHz / 300 Hz mixture had 1.60095 dB less HF-to-LF ratio than its quiet counterpart |
| Parameter metadata | One processor effect parameter, automatable, default 0; zero latency |
| State in fresh processors | 0/25/37/50/75/100% restored; invalid/out-of-range state rejected |
| Repeated preparation and sample-rate changes | Passed |
| Editor lifecycle and snapshots | Passed at 0/50/100%; 500 × 600 render inspected |
| Ordinary C++ heap allocation in 2,000 audio callbacks | Zero recorded |
| NaN/Inf/extreme input recovery | Finite output and no poisoned filter history |
| 200 processor/editor/state/snapshot lifecycles after warmup | Private memory grew 4,304,896 bytes, within 8 MiB bound |
| Two-minute stereo automation render | No instability/clipping/nonfinite output; 0.827868 seconds render time including generator/checks |
| Actual VST3 scan and instantiate | Passed; name YOUR HALO, vendor VoxxLab |
| Actual VST3 mono/stereo processing and host automation | Passed |
| Actual VST3 unload/recreate/state restore at a new rate | Passed |
| Actual VST3 editor creation/destruction | Passed |

The allocation hook observes ordinary C++ `new` in the linked processor, not every possible OS/CRT allocation. The memory-growth bound permits allocator/GUI caches and is not a leak detector. Timing is an offline throughput observation, not a live-monitoring CPU guarantee. These tests used generated signals, not recorded vocals.

## Remaining acceptance and design limits

- FL Studio itself was not launched for these tests. Verified scan, live monitoring, automation recording and `.flp` close/reopen must still be checked in FL Studio; the actual VST3 host test used JUCE.
- Listening and musical tuning on recorded vocals remain necessary. RMS compensation does not guarantee equal perceived loudness for every source.
- High-DPI mouse/keyboard interaction was not exercised in a live host, although the artwork is vector based.
- Saturation and peak safety are intentionally not oversampled. Hard drive can create harmonics/aliasing; normal-vocal regression input did not reach the safety knee.
- Width has a 12 dB/octave low-frequency rolloff, not a brick-wall cutoff. It preserves existing side information rather than forcing all bass to mono.
- Exact zero bypass preserves overloaded finite input; active peak safety is not an intersample true-peak limiter.
- No sanitizer, external plugin validator or extended multi-hour DAW soak was run. The tests detected no crashes or unstable samples in their coverage; the bounded memory test does not establish absence of all memory defects.

The `Docs/FL-Studio.md` checklist records the remaining DAW steps. No FL Studio installation or unrelated project was modified.
