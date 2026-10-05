# YOUR HALO engineering notes

The processor owns the APVTS and a separate, allocation-free audio engine. The editor owns only visual components and a parameter attachment. The single host parameter is `halo` (version 1), expressed as 0–100 percent with a default of zero.

The visual-development upgrade adds a read-only `HaloAudioMeter` beside the engine and a 30 FPS orb in the editor. It does not change this audio architecture or the mapping below. Meter values have no feedback path to the DSP. See [the visual report](VisualDevelopment/Report.md).

Design targets: minimum-phase filters, no delay lines, no lookahead, zero reported latency, 35 ms sample-by-sample parameter smoothing. All filter coefficients and detector time constants derive from the current sample rate. Mono and stereo input/output layouts match; mono does not synthesize a second channel.

Exact zero is a dry path once the smoothing ramp finishes. During a return to zero the enhancement fades away smoothly. Filter and detector histories continue tracking the dry signal to avoid cold-start transients on a later turn of the knob.

Validation distinguishes synthetic regression tests, actual VST3 host loading, and listening/DAW acceptance. Passing a synthetic test is not evidence that a vocal sounds good or that FL Studio project recall has been exercised.

## HALO mapping

Let `h` be the smoothed HALO value divided by 100. Both EQ stages use a maximum-boost filter in parallel with its input; the difference is blended by the stated power of `h`. This keeps coefficients fixed during automation and all states warm at zero.

| HALO | Presence at 3.4 kHz | Air shelf asymptote | Saturation drive | Saturation blend | Added high-frequency side |
| --- | --- | --- | --- | --- | --- |
| 0% | 0 dB | 0 dB | 1.000× | 0% | 0% |
| 25% | +0.52 dB | +0.71 dB | 1.163× | 0.75% | 0.5% |
| 50% | +1.11 dB | +1.59 dB | 1.325× | 3% | 2% |
| 75% | +1.71 dB | +2.51 dB | 1.488× | 6.75% | 4.5% |
| 100% | +2.30 dB | +3.40 dB | 1.650× | 12% | 8% |

The presence column describes the presence stage alone at its centre frequency, and air describes the shelf's upper asymptote. The full chain includes overlapping responses, level matching and dynamic protection; these values are not net output boosts.

- **Presence:** RBJ bell at 3.4 kHz, Q 0.65, maximum +2.3 dB; blend `h^1.15`.
- **Air:** RBJ high shelf at 10 kHz, slope 1, maximum +3.4 dB; blend `h^1.25`.
- **Glow:** `tanh(drive*x)/drive`, drive `1 + 0.65*h`, blended by `0.12*h²`. The transfer is odd and has unity small-signal gain; it does not add DC bias. No oversampling is used, so this stage is deliberately mild.
- **Protection:** dry-input 7 kHz bandpass sidechain, Q 0.9. Linked band/full-power followers use 3 ms attack and 90 ms release. Severity is `clamp((bandEnvelope - 0.045)/0.12) * clamp((band/full ratio - 0.28)/0.45)`. A smoothed severity blends a 6.5 kHz shelf of up to −2.8 dB by `h*severity`. It requires both substantial absolute HF level and HF dominance.
- **Width:** add/subtract `0.08*h²` times the existing side signal through a 300 Hz second-order Butterworth highpass. The extra side cancels in mono. Low-frequency enhancement rolls off at 12 dB/octave; this is not a brick-wall crossover, and existing low-frequency stereo information is preserved. Mono does not run this stage.
- **Compensation:** linked dry/wet power followers (250 ms), followed by a 150 ms smoothed gain ratio restricted to 0.76–1.0. The applied gain is `1 + h*(matchedGain - 1)`, preserving a smooth dry endpoint. This is RMS matching, not a perceptual loudness meter; source-dependent changes remain possible.
- **Peak safety:** a common stereo gain applies a tanh knee at `1 - 0.1*h`, with margin `0.09*h`. At full HALO the knee starts at 0.9 and the asymptotic ceiling is 0.99 (approximately −0.087 dBFS). At lower settings it approaches unity. No lookahead means the knee can generate harmonics when driven hard. Exact zero intentionally bypasses safety and preserves already-clipped input.

Double-precision filter state, bounded input recovery, finite parameter guards and tiny-state flushing protect stability. The processor enables scoped denormal suppression. Coefficients and time constants are recreated in `prepareToPlay`; callbacks allocate no scratch memory. Invalid state types and out-of-range HALO values are rejected. Only the parameter is recalled; filter/detector histories are reset on preparation, as is usual for a live effect.
