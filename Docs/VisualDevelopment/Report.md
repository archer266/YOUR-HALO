# YOUR HALO orb · Visual-development report

Implemented on 4 October 2026. This is a visual-development build, still version **1.0.0**, with the existing manufacturer/plugin codes, bundle ID, parameter ID and state format. No 1.1.0 release or new package was created. The original `dist` bundle and ZIP are retained unchanged.

## Files changed

- `Assets/haloLogo.png`: exact supplied original, copied into the project.
- `CMakeLists.txt`: JUCE BinaryData artwork target, orb source, and visual-validation test registration. Audio/plugin metadata and JUCE version are unchanged.
- `Source/PluginProcessor.h/.cpp`: read-only meter member and getter, input observation before the existing engine call, and meter reset during prepare/reset/release. Parameter/state/bus code is unchanged.
- `Source/Metering/HaloAudioMeter.h`: new sample-rate-aware input RMS/peak observer and atomic snapshot publication.
- `Source/UI/HaloOrbAnimation.h`: message-thread envelope, parameter-light smoothing, idle phase and micro-pulse model.
- `Source/UI/HaloOrbComponent.h/.cpp`: JUCE Slider-derived parameter control, Timer, drawing, cached lighting, hover/focus treatment and hit region.
- `Source/PluginEditor.h/.cpp`: orb attachment/layout and embedded official-logo header.
- Removed the superseded `HaloKnob` and `HaloLookAndFeel` files. Their interaction behavior is retained by the Slider-derived orb.
- `Tests/HaloVisualTests.cpp`, `Tests/HaloTests.cpp`, `Tests/Fixtures/HALO-1.0.0-37.vst3state`: preservation comparison, metering, control, scaled-preview and concurrent-render validation.
- `README.md`, architecture/verification notes, this report, baseline hashes, logs and previews document the current development build separately from the retained release.

**Neither `Source/DSP/HaloEngine.cpp` nor `.h` was modified.** Their SHA256 values are recorded in `Baseline-hashes.json` and checked again after development. The same audit checks the retained release files and exact logo copy.

## Audio → GUI

`processBlock` observes the existing input buffer without writing it, then invokes the existing DSP exactly as before. Metering uses a 25 ms sample-power follower and square root once per block, with stereo power averaged across channels. Peak information has a 120 ms hold/release. This is an inexpensive visual meter, not a calibrated loudness meter.

The audio thread publishes RMS, peak and a freshness counter through relaxed atomics. Compile-time checks require these atomics to be always lock-free on the target. There are no GUI locks, callbacks, FFTs or audio-thread allocations. Snapshots need not share an exact sample age: they only drive smoothed light. The meter follows input even at HALO zero, so the orb can react faintly while enhancement is off.

The editor only reads those atomics. It cannot change meter/DSP histories. Closing the editor destroys/stops its Timer without changing processing. Opening it again starts a new visual envelope with the recalled HALO brightness already initialized.

## Appearance and movement

The official image is embedded by `juce_add_binary_data` and decoded through `ImageCache`. The header displays its original wordmark region with feathered drawing overlays; the supplied PNG is not redrawn or altered on disk. No runtime path to the Desktop image is needed.

The orb itself is JUCE drawing: pearl shading, off-axis highlight, gold illumination, muted lavender/cyan/peach reflections, a softly feathered bloom and an almost circular aura with short drifting light arcs. Static surface/bloom images are cached at the drawing context's pixel scale and rebuilt only when dimensions/DPI change. Paths reserve and reuse storage. Animated washes and arcs stay on the message thread. Particles and a separate HF shimmer meter were omitted to keep the presentation restrained and the audio observer small.

HALO determines baseline illumination and reflection strength. The GUI combines 86% RMS and 14% peak, maps −52 to −6 dB into visual activity, and smooths activity with **65 ms attack / 320 ms release**. Activity subtly raises bloom and internal motion. HALO zero substantially suppresses the visible response; high HALO stays luminous in silence and becomes more radiant on phrases.

Idle motion never stops while the editor is showing: the light drifts on a roughly 18–26 second cycle in silence. Radius changes stay below **2.6%**, including idle breathing. No instantaneous peak is mapped directly to size. If the host stops callbacks without clearing the last meter, a 350 ms freshness timeout allows the GUI to settle rather than freeze brightly. Hidden editors skip updates; destroyed editors have no Timer.

Interaction remains vertical drag, wheel, double-click reset and arrow keys, using the existing APVTS `SliderAttachment`. Hover/keyboard focus softly brighten the edge; the cursor indicates vertical adjustment. Numeric percentage and HALO label remain beneath the sphere. The editor stays fixed at **500 × 600**.

## Build and validation

Release Windows x64 VST3 built successfully with the existing JUCE/MSVC/CMake setup. Meter, orb, visual refinement and final changes were compiled independently. The final build has no compiler warnings/errors. Final CTest: **2/2 suites passed in 25.88 seconds**. See `visual-final-build.log`, `visual-final-tests.log` at the project root and [detailed test output](Test-output.log).

- Retained 1.0.0 vs updated VST3: **4,869,360 samples matched exactly** across 40 HALO/rate/layout combinations, silence, changing buffers, automation and repeated editor open/close/reopen.
- Old VST3 saved state, persisted 37% fixture, host parameter count, plugin identifiers/version and zero latency passed.
- The existing 40-case DSP, loud input, NaN/denormal, realtime-allocation, two-minute playback and host-loading suite passed unchanged.
- RMS/peak accuracy and decay passed at 44.1/48/88.2/96 kHz, mono and stereo. Meter lifecycle/freshness passed.
- Parameter attachment, DAW-side value updates, vertical drag, wheel, keyboard, double-click and hit region passed with simulated JUCE input events.
- Real processor input drove 20 rendered conditions: HALO 0/25/50/75/100%, each with silence/quiet/normal/strong audio.
- Scale-factor renders passed at 100/125/150/200%, including pixel-dimension checks; surface caches followed the graphics scale.
- A real JUCE Timer ran concurrently with paced 64-sample audio through an offscreen native editor peer. Every audio sample matched the unchanged engine while painting; no nonfinite output/crash occurred. Activity rose during input and settled after silence. This is not an audio-device underrun test.

## Performance observation

Observed **29.69 FPS** for the 30 FPS timer. Full-editor offscreen paint median/p95: **1.95 / 3.65 ms**. Real repaints are limited to the orb area; the benchmark repainted the entire editor, including the header.

64-sample processor callback median/p95: **4.9 / 8.6 µs**. The unchanged engine's median was **4.2 µs**; the observed difference including metering and processor overhead was about **0.7 µs per block**. The callback budget at 48 kHz / 64 samples is 1.333 ms.

Median full-editor render time at 30 FPS corresponds to roughly 58 ms of render wall time per second. This is **not** a measured CPU/GPU utilization percentage. Measurements used JUCE's default offscreen image backend and a synthetic signal; hardware, host repaint scheduling, graphics backend and system load can change costs. Raw values are in [UI-performance.txt](UI-performance.txt).

## Build output and previews

Updated development VST3:

`C:\Users\Admin\Desktop\Coding Projects\YOUR HALO\build\YOUR_HALO_artefacts\Release\VST3\YOUR HALO.vst3`

Copy the full bundle only when installing this development build for DAW testing. No install or replacement of the user's FL Studio plugins was performed.

- [Interactive still/animation preview](Preview.html): local HTML displaying actual JUCE captures, with silent 90% HALO phrase animation at 30 FPS.
- [20-condition contact sheet](Orb-contact-sheet.png).
- [100% HALO with normal vocal activity](Orb-HALO-100-2.png).
- [0% HALO in silence](Orb-HALO-0-0.png).
- [200% scaled capture](Orb-DPI-200.png); other scale captures are alongside it.
- `AnimationFrames`: 120 unmodified JUCE screenshots used by the preview; no external network/assets required.

## Remaining visual/DAW checks

Actual FL Studio scanning, recorded-vocal listening, automation recording, `.flp` reopening, live device dropout/CPU testing and physical multi-monitor Windows DPI interaction remain manual acceptance steps. Scaled screenshots establish drawing behavior, not live host scaling or mouse-coordinate correctness across monitors.

The artistic balance may still benefit from review on a bright and a dim display, especially subtle pearl reflections, small caption contrast and perceived bloom strength. The official artwork's cloud background is feathered into the header, so inspect that transition on those displays. The orb intentionally has no particles, literal waveform or separate HF shimmer. The visual build has not been packaged as a final release.
