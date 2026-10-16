# Changelog

## 1.0.0 — 2026-10-15

- Windows x64 VST3 vocal enhancement with a single HALO amount orb and the existing AURA, GLOW, SHINE and ASCEND character selectors.
- Original compression, parallel density, harmonic saturation, presence/air shaping, dynamic harshness protection and high-passed existing-side enhancement.
- Exact neutral output at settled HALO zero, 35 ms macro smoothing, smoothed mode transitions and zero reported latency.
- Pearl/gold interface with embedded HALO artwork and fluid, audio-reactive light animation.
- Automated mono/stereo validation at 44.1, 48, 88.2, 96, 176.4 and 192 kHz, varied buffers, automation, state recall and editor lifecycle checks.
- Windows installer with uninstall support, clean manual-install ZIP and SHA-256 checksums.

The nonzero-HALO sound includes the previously developed four-mode engine and differs intentionally from the original early prototype. Existing HALO parameter and plugin identifiers are preserved. Older sessions without MODE restore AURA.

### Limitations

- Windows x64 VST3 only; no standalone app, VST2, AU or macOS build.
- Installer and plugin are unsigned.
- Native DAW GUI, real-vocal listening and per-monitor hardware-DPI acceptance remain untested for this release. See the release QA report for the automated host and rendering coverage.
- Peak protection controls sample peaks; it is not a true-peak mastering limiter. Nonlinear processing is not oversampled.
