# YOUR HALO v1.0.0 release QA

Date: October 15, 2026. Windows x64 Release, JUCE 8.0.12, plugin version 1.0.0. No audio-engine, parameter, plugin-identifier or interface changes were made during finalization. Existing four-mode character, the HALO orb, zero reported latency and 35 ms smoothing are retained. There are no new controls.

## Executed checks

- Release compilation succeeded. Version and VoxxLab metadata were confirmed by the actual VST3 host and pluginval.
- Core regression suite passed: 240 mode/rate/layout/HALO combinations across 44.1/48/88.2/96/176.4/192 kHz, mono/stereo and buffers 1/17/64/127/256/512. Exact settled-zero behavior, finite output, DC limits, bounded active peaks, silence/tail decay, denormal suppression, zero latency, 35 ms smoothing, rapid automation and block partition invariance passed.
- All 16 mode transitions passed at all six rates and both layouts. Valid old states default to AURA, both current parameters save/restore, malformed state is rejected, and actual VST3 scan/load/process/reopen/state checks passed.
- Audio-thread ordinary C++ heap-allocation count remained zero. Repeated processor/editor lifecycles and 120-second automation playback passed. This is regression and memory-growth coverage, not an exhaustive race detector or heap-profiler proof.
- Expanded visual suite passed after correcting a test-duration assumption: its former fixed 4,000-block silence interval was too short at 176.4/192 kHz. It now waits two real seconds. Product code did not change. Core CTest passed in 32.91 seconds; the corrected visual CTest passed in 94.59 seconds.
- Compatibility comparisons against both retained pre-upgrade binaries passed across 60 settings/rates/layouts per comparison (7,304,040 samples each), confirming identifiers, old state, zero latency and the intended dry endpoint. Nonzero audio intentionally retains the previously developed four-mode sound, not the early prototype.
- Offscreen UI tests passed: control attachments/gestures, mode selection, 20 audio/HALO states, four mode captures, 100/125/150/200% render sizes, 1,440 animation frames, stopped-host decay and 120-second phase continuity at 30/45/60 FPS. No flicker/crash/repaint failure was detected by these checks; native hardware composition was not tested.
- Tracktion **pluginval v1.0.4**, strictness **5**, passed independently at all six rates and block sizes 1/17/64/127/256/512/1024. Seed: 20261015. GUI tests were explicitly skipped. The separate Steinberg validator was not installed and was not run; pluginval's own log states that skip.
- Final installer compiled with Inno Setup and passed a silent, isolated **per-user command-line** installation test. Both VST3 payload files matched the Release build by SHA-256. Instructions, dependency notices and uninstaller were present. The full core/installed-host suite passed against the installed bundle. Uninstall removed the bundle and temporary HKCU registration. Default all-users/UAC installation was not exercised.
- The ZIP was expanded into a fresh verification directory, every file was compared with staging, and its VST3 SHA-256 matched the validated Release binary. Only the complete bundle, README, CHANGELOG, INSTALL instructions and dependency notices are distributed.

## Performance observations

Concurrent 48 kHz / 64-sample audio and the nominal 60 FPS timer observed **59.6875 FPS**, median/p95 full-editor offscreen paint **3.0459/4.8794 ms**, and median/p95 audio callback **15.8/27.9 µs**. The callback budget at 48 kHz is 1,333 µs. Offscreen timing is not a native GPU utilization measurement and varies with system load.

Standalone full-HALO callback benchmarks at 176.4/192 kHz remained around 14.2–14.7 µs median. Estimated thread-CPU duty across the six tested rates remained below approximately 5% of one core in this run. See the reproducible [CPU measurements](../DSPUpgrade/CPU.csv). These are harness observations, not guarantees for every DAW/computer.

## Package identities

- VST3 binary SHA-256: `eb26d4e78e7bb6dbff91d27ac4402e59f1bd2e0fbc543e4c446c7d367ff4a130`
- ZIP SHA-256: `fcd57ad6545a037a0c65b7f6f27cfb3aa6483e1eaccf297fce64bc49c5e2d25f`
- Installer SHA-256: `d6899d2d8c6746cc2d76bd00f71bc0f217214d4ab4f0ea67eb9c154faa9e600c`

The source commit and `v1.0.0` tag are recorded by the GitHub Release. Compiled outputs remain ignored and are uploaded as release assets rather than source files.

## Remaining limitations / not performed

- No computer-use or desktop UI automation was performed, as requested. FL Studio 21/2024 are installed but were not opened or tested. Native FL Studio, REAPER, Ableton Live, Cubase and other DAW project/session acceptance remain untested; actual VST3-host automation/state checks passed in the JUCE harness.
- Real recorded-vocal listening, physical audio-device behavior, native per-monitor DPI transitions, visual flicker observation and Windows all-users/UAC installation remain manual acceptance checks.
- Pluginval GUI tests, Steinberg's separate validator, dedicated race sanitizers and exhaustive memory profiling were not run. A test-only WAV-writer deprecation warning occurred on the initial test compilation; it does not affect the plugin runtime.
- Installer/plugin are unsigned. Windows x64 VST3 only. No new platform formats are supplied.
- No lookahead or oversampling; peak protection is sample-peak soft protection, not true-peak mastering limiting. Existing-side width does not create stereo information from dual-mono input.
- Full historical binary-comparison tests require the retained local prototype reference in `dist`, which is deliberately excluded from Git. Public source builds can run the core suite without that reference; the runtime plugin does not require it.

Evidence: [core and installed VST3 host output](Core-and-installed-host-tests.txt), [visual/compatibility output](Visual-tests.txt), [pluginval output](Pluginval.txt).
