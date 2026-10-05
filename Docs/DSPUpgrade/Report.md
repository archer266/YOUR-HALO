# YOUR HALO four-mode DSP and fluid-light upgrade

Development build completed and tested on Windows x64, October 4, 2026. The sound intentionally changes. This is an original VoxxLab enhancement chain, with four automatable modes and the existing HALO orb macro. The original embedded HALO artwork, plugin identifiers and `halo` parameter ID remain. Plugin version remains 1.0.0 during development; the original release in `dist` is untouched. No final public release was packaged or installed.

## Architecture

Input → mode-specific low-mid/body shaping → stereo-linked RMS soft-knee compressor plus independent parallel density branch → band-limited harmonic saturation → presence bell / air shelf / HF harmonic excitement → dynamic harshness protection → existing-side stereo enhancement → common stereo soft peak protection → output.

The main compressor and parallel branch share a linked, 90 Hz high-passed detector; the louder channel drives both, maintaining stereo balance. A common mode-morphing engine interpolates control values rather than rebuilding IIR coefficients or resetting histories. Fixed maximum-response filters are blended with their inputs and continue tracking even at HALO zero. HALO smooths over 35 ms; mode weights crossfade over 60 ms, including switches during a previous transition.

The old automatic RMS attenuation was removed intentionally. Density comes from compression, parallel detail lift, tone shaping and nonlinear harmonic processing, then controlled makeup. It is not a simple output-gain macro.

## Mode settings

These are the full-HALO endpoints. Intermediate processing follows the nonlinear mappings below.

| Mode | Main ratio | Threshold | Gain attack / release | Makeup profile | Parallel addition | Saturation drive / mix | HF excitement | Extra existing side |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| AURA | 3:1 | −24 dBFS | 22 / 120 ms | 6 dB | 22% | 3.4× / 36% | 10% | 13% |
| GLOW | 3.5:1 | −25 dBFS | 30 / 160 ms | 6.5 dB | 35% | 4.8× / 58% | 4% | 8% |
| SHINE | 2.7:1 | −23 dBFS | 12 / 85 ms | 5.5 dB | 18% | 3.0× / 28% | 24% | 20% |
| ASCEND | 4:1 | −27 dBFS | 16 / 100 ms | 8 dB | 50% | 4.4× / 52% | 20% | 30% |

The compressor uses a **6 dB soft knee**, an 8 ms power detector and separate attack/release smoothing of gain reduction. There is no lookahead. The parallel branch has its own 6:1 / −30 dBFS curve, 6 dB knee, 15 ms gain attack, 180 ms release and 8 dB makeup. It is added to the main path, weighted by HALO and mode, so quiet details become more present instead of simply replacing the clean path.

Designed audible distinctions:

- **AURA:** balanced and modern; smooth compression, controlled presence, airy polish and modest width.
- **GLOW:** fuller at 240 Hz, less cleanup, the richest warm saturation, slower leveling and a gentler high end.
- **SHINE:** most open air and forward presence, faster/cleaner compression, lighter body saturation and stronger HF protection.
- **ASCEND:** strongest ratio, deepest threshold and largest parallel addition; substantial quiet-detail density, rich saturation, strong presence/air and widest existing side information.

The synthetic tests confirm meaningful differences after normalizing gain: pairwise mode residuals at full HALO range from **7.29% to 26.77%**. These describe the designed and measured processing character; final vocal listening acceptance remains manual.

## HALO mappings

Let `h` be the 35 ms smoothed macro, normalized to 0–1; let `d = h^0.90`.

- Main ratio: `1 + d × (modeRatio − 1)`.
- Main threshold: `−10 + d × (modeThreshold + 10)` dBFS.
- Applied main reduction: the smoothed compressor reduction multiplied by `d`, gently fading both compression and residual envelope memory at zero.
- Main makeup: `modeMakeup × h^1.20` dB.
- Parallel addition: `modeDensity × h^1.10 × gain(8 − parallelReduction)`.
- Low-mid/body shaping and presence: `h^1.10` times the selected filter blend.
- Air and HF excitement: `h^1.35`.
- Saturation drive: `1 + h^1.15 × (modeDrive − 1)`; nonlinear residue blend is `modeSaturation × h^1.15`.
- Added high-passed side: `modeWidth × h^1.60`.

These curves bring dynamics and density in earlier, while air, saturation and width rise more gradually. At settled HALO zero, every mode returns the original finite input exactly, including an already-overloaded input; invalid/nonfinite input is sanitized rather than propagated.

## Tone and saturation

The fixed tone filters are a +3 dB / 240 Hz / Q 0.75 body bell, −2.5 dB / 380 Hz / Q 0.8 cleanup bell, +5 dB / 3.4 kHz / Q 0.7 presence bell and +6.5 dB / 9.5 kHz air shelf. Each mode blends a fraction of the fixed filter with its input. The following are isolated-stage approximate full-HALO responses, not net chain boosts:

| Mode | Body at 240 Hz | Cleanup at 380 Hz | Presence at 3.4 kHz | Air upper asymptote |
|---|---:|---:|---:|---:|
| AURA | 0 dB | −1.31 dB | +3.51 dB | +4.35 dB |
| GLOW | +2.57 dB | −0.36 dB | +2.15 dB | +2.33 dB |
| SHINE | 0 dB | −1.94 dB | +4.61 dB | +6.14 dB |
| ASCEND | +1.12 dB | −0.92 dB | +4.20 dB | +5.77 dB |

Filter blending interpolates linear response, not decibels, so these stage responses differ slightly from the numerical profile weights in the source. Overlapping filter phase/response, compression, saturation and protection determine the actual output.

The nonlinear stage first low-passes a duplicate at 4.2 kHz, applies the odd unity-small-signal curve `tanh(drive × x) / drive`, subtracts the duplicate, low-passes that nonlinear residue at 10.5 kHz and blends it into the main path. Gain compensation is `1 + 0.12 × h^1.15`, at most about +0.98 dB. A separate 3.5 kHz high-passed branch adds the residue `x − tanh(2.8 × x)/2.8`, low-passed at 11 kHz, for mode-specific excitement. Frequencies clamp below Nyquist at preparation. Odd transfer curves avoid built-in DC bias.

This is band-limited, zero-buffer-latency nonlinear processing, without oversampling. Band limiting restrains upper harmonics; it does not guarantee alias-free saturation. Aggressive peak protection can also generate harmonics. No proprietary third-party enhancer algorithms or presets were used.

## Harshness, width and peaks

The existing dry-input linked HF detector is stronger: 7 kHz bandpass / Q 0.75, 2 ms attack and 80 ms release. Severity requires both an absolute band envelope and a high band/full-envelope ratio:

`clamp((bandEnvelope − 0.025)/0.10) × clamp((band/full ratio − 0.22)/0.42)`.

A smoothed severity blends a fixed −8 dB / 5.5 kHz shelf into the enhanced signal. Maximum profile weights are 5.5/8 for AURA, 4.5/8 for GLOW, 7.5/8 for SHINE and 8/8 for ASCEND, multiplied by `h^1.10`. Quiet air and low-frequency vowels do not receive a constant de-essing cut. The strong 7 kHz/300 Hz test measured **3.59 dB more relative HF attenuation** than its quiet counterpart in AURA.

Width adds only existing side information through a **300 Hz, 12 dB/octave high-pass**; it progressively suppresses low-frequency widening rather than using a brickwall crossover. AURA/GLOW/SHINE/ASCEND add up to 13/8/20/30%. Mono inputs never synthesize side information; dual-mono and antisymmetric mono cancellation pass in all four modes. Equal/opposite side additions preserve the mono sum before the common safety gain.

Peak protection uses one common stereo gain, with knee `1 − 0.15h` and tanh margin `0.10h`. Its sample-peak asymptote is `1 − 0.05h`: **0.975 at HALO 50% and 0.95 at 100%** (about −0.22 and −0.45 dBFS). It has no lookahead or delay. This is gentle sample-peak protection, not a true-peak mastering limiter. Reported latency remains **0 samples**.

## A/B measurements

Reproducible 10-second, 48 kHz mono synthetic vocal-like fixture, with normal, quiet, loud and intermediate sections. Initial one second excluded from measurements. RMS and peaks are sample statistics, not perceptual LUFS; the files retain actual gain rather than forced loudness matching.

| Mode | HALO | RMS change | Peak change | Mean / max main GR | Crest change | Quiet/loud detail gain |
|---|---:|---:|---:|---:|---:|---:|
| AURA | 50% | +2.14 dB | +3.93 dB | 0.33 / 1.87 dB | +1.79 dB | +2.78 dB |
| AURA | 100% | +1.68 dB | +3.76 dB | 2.56 / 8.64 dB | +2.08 dB | +8.99 dB |
| GLOW | 50% | +2.87 dB | +3.98 dB | 0.57 / 2.54 dB | +1.12 dB | +4.32 dB |
| GLOW | 100% | +1.54 dB | +3.76 dB | 4.17 / 11.54 dB | +2.22 dB | +13.25 dB |
| SHINE | 50% | +2.02 dB | +3.88 dB | 0.25 / 1.59 dB | +1.86 dB | +2.30 dB |
| SHINE | 100% | +1.96 dB | +3.76 dB | 1.96 / 7.48 dB | +1.80 dB | +7.60 dB |
| ASCEND | 50% | +3.05 dB | +3.99 dB | 0.62 / 2.94 dB | +0.94 dB | +4.88 dB |
| ASCEND | 100% | +2.19 dB | +3.76 dB | 4.48 / 12.76 dB | +1.57 dB | +13.51 dB |

Quiet/loud detail gain compares processed-versus-dry RMS gain in settled quiet and loud fixture sections. Main GR excludes the separate parallel compressor and peak protection. Whole-file crest factor **increased** on this fixture: sudden quiet-to-loud jumps produce attack overshoots while quiet details are lifted. The plugin therefore should not be presented as flattening every transient or universally reducing crest factor. Peaks remain bounded by the safety curve.

After fitting and removing the best static output gain, the processed-versus-dry residual is **11.88–17.63% at HALO 50%**, rising to **34.55–45.27% at 100%**. This demonstrates actual tone/dynamic/nonlinear change beyond gain. [Raw measurements](Loudness.csv) and [dry/processed audio review](Review.html) are included.

## UI and fluid motion

Four small pearl/champagne mode buttons sit below the unchanged hero orb. Host automation updates selection; button gestures update the host parameter. AURA retains warm pearl, GLOW introduces extra champagne warmth, SHINE uses faint cool pearl and ASCEND adds white-gold radiance. Their visual transitions smooth over 150 ms.

The latest visual direction is implemented as original flowing aurora light: broad feathered veils, overlapping gold/pearl/pale-pink/lavender/cyan clouds and a core with changing aspect, position and shear. Defined rotating stripes, aura tracks and moving ring outlines were removed. Independent phases continuously deform the fields, with rear-to-front fading and a stable shaded glass shell. There is no imported assistant artwork or copied palette/geometry/animation.

The 60 FPS animation model, independent phase wrapping, lock-free input meter, 1×–2.1× audio speed range, 4.4% bounded expansion and idle life remain. The silent internal pixel-change test measured **4.42417/255** over two seconds and passed its stronger-motion threshold. The [24-second captured preview](Preview.html) includes six seconds idle, four quiet, four normal, four strong and six return-to-idle, at HALO 90%. It is silent and uses actual JUCE renders.

## Build, validation and performance

Release build succeeded. **CTest 2/2 passed**, core 20.63 seconds and visual/host 80.92 seconds, total 101.56 seconds. Core completed **126,313,750 assertions**:

- All 160 combinations: four modes × five HALO values × mono/stereo × 44.1/48/88.2/96 kHz; varied 1/17/64/127/256/512-sample buffers.
- Silence, loud input, bounded peaks, finite output, DC bounds, denormal suppression, long tail decay and filter stability.
- Exact settled HALO-zero output, four-mode block-partition invariance, rapid HALO/MODE automation and return-to-zero continuity.
- All 16 mode transitions at every sample rate and mono/stereo; worst first-sample control discontinuity **0.0000379**. HALO control discontinuity **0.000208**. These isolate the control change from normal waveform motion and intentional saturation.
- Two-parameter save/restore, original states defaulting to AURA even after ASCEND, malformed/range-invalid/nonfinite mode state rejection, rate changes and editor lifecycles.
- Actual VST3 host tests for the 160 mode/amount/rate/layout cases, plus automation, state recall, unload/reopen and editor loading.
- **Zero ordinary C++ heap allocations** detected in the audio callback; nonfinite input recovery; 120 seconds automated playback.

The visual/host suite also checked compatibility against both retained release and immediately previous development VST3s: 4,869,360 samples per comparison, preserving IDs, legacy states, zero latency and the exact dry endpoint while confirming the intentional stronger audio difference. It tested the controls, meter, 20 input/HALO captures, four mode captures, four DPI sizes, all 1,440 animation frames, 120-second six-phase continuity at each 30/45/60 FPS and concurrent timer/audio runs.

At nominal 60 FPS the harness observed **58.75 FPS**, with median/p95 full-editor offscreen rendering **2.0571/3.1716 ms**. Concurrent 48 kHz / 64-sample stereo audio callback median/p95 was **12.7/21.1 µs**, versus the previous engine's approximately 4.9 µs median. The upgraded engine alone measured 11.3 µs median. The stronger processing is about 2.6× the previous callback cost, remaining well below the 1,333 µs block budget.

Separate full-HALO, all-mode benchmarks measured 48 kHz medians **14.2–15.1 µs** and p95 **14.3–15.3 µs**. Windows thread-CPU duty was approximately **1.03–1.17% of one core at 48 kHz**, and **0.94–2.15% across the four rates**, including buffer copy/timing overhead. This is offline CPU time divided by the equivalent audio duration, not a DAW's whole-process CPU meter. [CPU.csv](CPU.csv) contains all 16 cases. Median paint-only wall time at 60 FPS corresponds to about 12.34% of one core; it is not a GPU/CPU utilization measurement. Host hardware and scheduling can differ.

Native FL Studio project acceptance and real recorded-vocal listening remain manual. Synthetic tests and the JUCE hosting harness do not establish subjective final sound quality. Preview JavaScript syntax and local resources are verified; browser playback is supplied for manual review.

## Compatibility, changed files and output

`halo` remains version 1 and the first parameter, 0–100%, default 0. `mode` is an appended version-1 choice: 0 AURA, 1 GLOW, 2 SHINE, 3 ASCEND. Old valid states without mode get explicit AURA. Plugin name, manufacturer, bundle ID, manufacturer/plugin codes and development version retain their existing values. Sound at nonzero HALO changes intentionally.

- `Source/DSP/HaloEngine.h/.cpp`: new compressor/density/saturation/tone/protection engine and mode morphing.
- `Source/PluginProcessor.h/.cpp`: appended MODE parameter, processing input and validated legacy/new state recall.
- `Source/PluginEditor.h/.cpp`: four elegant attached mode selectors.
- `Source/UI/HaloOrbComponent.h/.cpp`: subtle mode radiance and organic aurora/cloud rendering.
- `Tests/HaloTests.cpp`, `Tests/HaloUpgradeChecks.h`, `Tests/HaloVisualTests.cpp`: expanded DSP, compatibility, A/B, CPU, control, state and visual checks.
- `CMakeLists.txt`: current test artifact destinations; plugin identifiers unchanged.
- `README.md`, `Docs/Architecture.md`, `Docs/Verification.md`, `Docs/FL-Studio.md`, `Docs/DSPUpgrade`: documentation, reproducible logs, measurements, audio and previews.

Updated development bundle: **`build/YOUR_HALO_artefacts/Release/VST3/YOUR HALO.vst3`**. Preserve the complete bundle, including `Contents`.

The [preservation audit](Preservation-audit.json) verifies the original logo, lock-free meter, animation model and retained release binary/ZIP; CMake's expected difference is only the test destination. Previous audio-engine source, architecture and development bundle are retained here for review. [Build log](Build-output.log), [CTest summary](CTest-summary.log), [full test output](CTest-output.log), [performance](UI-performance.txt).
