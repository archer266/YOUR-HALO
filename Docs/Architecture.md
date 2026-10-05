# YOUR HALO current architecture

The current development build has the original `halo` macro plus an appended `mode` choice: AURA, GLOW, SHINE and ASCEND. Both are automatable. The original plugin identifiers and HALO parameter ID/version are retained; valid older states with no MODE value restore AURA. The sound intentionally changes at nonzero HALO.

The processor owns the APVTS, allocation-free audio engine and the existing read-only lock-free input meter. Processing is tone shaping ? stereo-linked soft-knee compression plus parallel density ? band-limited saturation ? presence/air/excitement ? dynamic harshness control ? high-passed existing-side enhancement ? soft sample-peak protection.

HALO smooths over 35 ms. Mode controls morph over 60 ms without resetting IIR or detector histories. Filter coefficients remain fixed between preparation calls; the macro blends their responses instead of abruptly changing coefficients. Settled HALO zero is exact dry output. Mono and stereo layouts must match; reported latency remains zero, with no lookahead or delay buffers.

The editor retains the 500?600 pearl/gold branding and embedded official logo. Four small mode buttons sit beneath the 60 FPS orb. Independent fields create fluid aurora veils, morphing pastel light and a stable glass sphere. Mode colors smooth separately on the GUI thread, and the existing input meter energizes motion without a feedback path to audio.

See the [current DSP and visual report](DSPUpgrade/Report.md) for exact mappings, compression, saturation, EQ, protection, width, sample-peak ceiling, CPU measurements, A/B results and compatibility details. The [previous architecture](DSPUpgrade/Previous-Architecture.md) describes the retained original engine, not the current development sound.
