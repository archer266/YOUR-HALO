YOUR HALO v1.0.0 is VoxxLab's Windows x64 VST3 vocal enhancer, with the HALO amount orb and four existing processing characters: AURA, GLOW, SHINE and ASCEND.

- Vocal tone shaping, smooth compression, parallel density, harmonic saturation, presence/air, dynamic harshness protection and controlled existing-side width.
- Neutral output at settled HALO zero, 35 ms smoothing, zero reported latency, mono/stereo support and parameter automation/session recall.
- Embedded HALO branding and a fluid pearl/gold audio-reactive interface.
- Finalization preserves the existing sound, controls and plugin identifiers; older states without MODE restore AURA.

**Install:** Close your DAW and run the Windows installer, or extract the ZIP and copy the complete `YOUR HALO.vst3` bundle to `C:\Program Files\Common Files\VST3\VoxxLab`. Preserve `Contents` and rescan VST3 plugins. README, CHANGELOG, installation instructions and dependency notices are included. Verify downloads against `SHA256SUMS.txt`.

**Validation:** Core and visual/host suites passed at six standard sample rates from 44.1–192 kHz. Pluginval strictness 5 passed command-line validation. ZIP extraction/payload hashes and isolated per-user installer/install-host/uninstall checks passed.

**Known limitations:** Windows x64 VST3 only; unsigned installer/plugin. Native DAW GUI and real-vocal listening, hardware-DPI transitions and all-users/UAC installation were not tested. Pluginval GUI tests and the separate Steinberg validator were not run. Peak protection is sample-based, not true-peak limiting; nonlinear processing is not oversampled. Detailed evidence is in `Docs/ReleaseQA/Report.md` in the tagged source.
