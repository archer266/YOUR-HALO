# YOUR HALO living energy sphere

Visual-development build validated on Windows x64, October 4, 2026. The shell and official embedded HALO branding retain the pearl/champagne identity. This pass substantially increases internal circulation and doubles the nominal GUI cadence to 60 FPS. Plugin version remains 1.0.0. No final release was packaged or installed.

## Animation layers

- **Energy cloud:** broad asymmetric gold/white/lavender/cyan light rotates clockwise, with independently changing shape and depth.
- **Counter-moving caustic bands:** two curved translucent bands circulate clockwise and counter-clockwise. Aspect deformation and moving centers prevent rigid-image rotation. These are incomplete flowing curves inside the sphere, not external rings.
- **Three orbiting pearl clouds:** tilted projected orbits change screen position, size, deformation and visibility. Rear light is dimmer; near light becomes more pronounced. The fast cloud remains broad and soft rather than a point.
- **Changing core:** three asymmetric cached luminous clouds overlap at different speeds, radii and aspect ratios. Their combined center changes shape continuously.
- **Independent surface reflection:** a feathered curved highlight travels around the glossy shell without rotating the shell itself. The spherical key lighting and rim remain stable.
- **Atmospheric aura:** centered bloom plus an offset, rotating/deforming elliptical bloom, gentle aura breathing and traveling soft edge light. Input expands and brightens it.

Depth is simulated using tilted orbits and a continuous front/rear visibility envelope. Each band crossfades between a low-resolution soft rear cache and a more defined front caustic; the front contribution follows the squared near-hemisphere envelope. This is optimized 2D compositing, not a volumetric 3D simulation.

## Rotation and audio response

| Motion phase | Idle speed | Idle revolution | Full-envelope speed | Full-envelope revolution |
|---|---:|---:|---:|---:|
| Slow cloud / band | 0.65 rad/s | 9.67 s | 1.365 rad/s | 4.60 s |
| Counter band / medium core | 1.10 rad/s | 5.71 s | 2.31 rad/s | 2.72 s |
| Fast pearl cloud / core drift | 1.95 rad/s | 3.22 s | 4.095 rad/s | 1.53 s |
| Surface reflection | 0.45 rad/s | 13.96 s | 0.5625 rad/s | 11.17 s |
| Atmospheric aura | 0.38 rad/s | 16.53 s | 0.589 rad/s | 10.67 s |

Three internal circulation phases use a separate **1.0×–2.1×** smoothed input-speed multiplier, with 250 ms acceleration / 700 ms deceleration. Surface and atmosphere accelerate more gently. All phases keep moving in silence and wrap independently without discontinuities. A sixth, slower 0.30 rad/s phase controls idle breathing.

HALO controls baseline radiance, core strength, gold/pearl energy visibility and aura strength. Smoothed input controls momentary brightness, circulation speed, band thickness, cloud size, bloom, highlight intensity and expansion. The existing lock-free RMS/peak meter is unchanged. Maximum size remains **4.4%** above the base radius, including the 0–0.5% idle breath; the size envelope retains 150 ms attack / 380 ms release and light retains 65 ms / 320 ms smoothing.

Rendered internal pixels in silence changed by a mean **4.23776/255** over two seconds, versus 1.55544/255 in the previous refinement (about 2.7×). This pixel comparison and visual capture inspection support the stronger movement; they do not replace judging the animation in a DAW.

## Frame-rate and performance results

Three separate 3.2-second runs used the actual JUCE timer with concurrent 48 kHz stereo / 64-sample audio processing. Rendering measurement covers the entire editor using the offscreen JUCE image backend, providing a conservative comparison with the orb-only repaint request.

| Requested FPS | Observed FPS | Median / p95 full-editor paint | Median / p95 audio callback |
|---|---:|---:|---:|
| 30 | 30.000 | 1.8589 / 3.0641 ms | 4.9 / 8.6 µs |
| 45 | 44.375 | 1.7630 / 2.9213 ms | 4.9 / 6.4 µs |
| **60 (selected)** | **59.6875** | **1.7697 / 3.1341 ms** | **4.9 / 8.5 µs** |

The previous refinement's median full-editor paint was 2.1470 ms at about 30 FPS. Cached clouds and caustics lowered per-frame median cost about 17.6% despite adding motion layers. At 60 FPS, median paint-only wall time estimates 10.62% of one core versus 6.44% for the previous 30 FPS build. This is not a CPU/GPU utilization measurement. The unchanged engine median remained 4.2 µs; wrapper/meter overhead remained about 0.7 µs. Tail timings vary with Windows scheduling.

Static spherical lighting, bloom, caustics, softer rear bands and luminous cloud textures are created only on size/DPI changes. Steady animation transforms and composites cached images and reuses preallocated paths. Existing logo caching is retained. Repaints are confined to the orb component; there is no OpenGL and no new work, locks or allocations on the audio thread. Native host/DPI scheduling can differ from the harness; FL Studio interactive acceptance remains manual.

## Validation

Release VST3 and test executable built successfully without reported compiler warnings/errors. Core CTest passed **1/1**, 4.56 seconds, **40,401,140 assertions**. The extended suite passed separately:

- Refined VST3 versus **original retained release:** 4,869,360 bit-identical samples.
- Refined VST3 versus **immediately previous refinement build** (`PreviousBuild.vst3`): another 4,869,360 bit-identical samples.
- Each comparison covered 40 rate/layout/HALO combinations, changing block sizes, silence, automation and editor open/close/reopen. Both reference builds retain version 1.0.0, so the two output lines use the same version label; their order is original then previous.
- Zero latency, mono/stereo, identifiers, HALO parameter identity, legacy state recall, automation and gain/output behavior preserved.
- Actual VST3 scan/load, editor, state, automation, unload/reopen and lifecycle checks passed in the JUCE hosting harness. Core audio-thread heap allocation count remained zero.
- Lock-free meter accuracy/freshness, silence decay, stopped-host recovery, UI drag/wheel/keys/reset and attachment checks passed.
- 20 actual-audio static states, four DPI sizes and all 1,440 preview frames exported successfully.
- 120 seconds of six-phase continuity at **each** 30/45/60 FPS passed; input speed reached 2.1×, maximum expansion reached 4.39929%, and speed/size transitions remained bounded and smooth.
- Real concurrent timer/audio runs passed at all three frame rates without observed crashes, repaint/lifecycle failures or audio mismatches.

The [SHA-256 preservation audit](Preservation-audit.json) confirms byte-identical DSP, processor, meter, editor, CMake, logo and retained release binary/ZIP. Source changes have no feedback path to audio, state or automation.

## Changed files and output

- `Source/UI/HaloOrbAnimation.h`: separate speed envelope, six continuous phases, stronger circulation; existing size envelope preserved.
- `Source/UI/HaloOrbComponent.h`: 60 FPS default, benchmark cadence selection and cached cloud/rear-band images.
- `Source/UI/HaloOrbComponent.cpp`: cached animated energy layers, projected depth, counter-circulation, moving specular and atmospheric bloom.
- `Tests/HaloVisualTests.cpp`: frame-rate-aware audio feeding, 24-second five-stage capture, comparisons against both reference builds, stronger idle-motion check, multi-cadence continuity and real timer/audio benchmarks.
- `README.md` and `Docs/SwirlDevelopment`: current build pointers, previews, screenshots, preservation hashes and logs.

Updated bundle: `build/YOUR_HALO_artefacts/Release/VST3/YOUR HALO.vst3`. Copy the entire bundle including `Contents`. The original final release in `dist` is untouched.

[24-second animated preview](Preview.html) · [Before/after](BeforeAfter.html) · [Strong-input screenshot](Orb-HALO-100-3.png) · [20-state contact sheet](Orb-contact-sheet.png)

Preview uses actual 360×432 JUCE captures at 60 FPS and HALO 90%, with six seconds idle, four seconds quiet, four normal, four strong and six release/idle. Synthetic vocal-like input passes through the real processor and meter; the preview is silent. It uses a small rolling frame cache rather than holding all decoded frames in memory.

Preview JavaScript syntax, local links and all sequential frame filenames were checked. Chrome and Edge headless processes failed to start in this restricted environment, so browser playback was not automatically verified. The underlying JUCE frame rendering and exports passed independently.

Evidence: [build log](Build-output.log), [core test output](Core-test-output.log), [visual/host output](Visual-test-output.log), [30 FPS](UI-performance-30.txt), [45 FPS](UI-performance-45.txt), [60 FPS](UI-performance-60.txt), [previous performance](Before-performance.txt).
