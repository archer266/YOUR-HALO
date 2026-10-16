# YOUR HALO v1.0.0 — VoxxLab

A Windows x64 VST3 vocal enhancer with a single HALO amount orb and four existing character modes: AURA, GLOW, SHINE and ASCEND. It combines tone shaping, smooth compression, parallel density, harmonic enhancement, presence/air, dynamic harshness control and controlled stereo-side enhancement.

Features include 35 ms HALO smoothing, neutral output at settled zero, zero reported latency, mono/stereo operation, parameter automation, legacy session recall and a pearl/gold audio-reactive interface with embedded artwork.

## Install

Close your DAW. Copy the complete `YOUR HALO.vst3` folder to:

`C:\Program Files\Common Files\VST3\VoxxLab\YOUR HALO.vst3`

Rescan VST3 plugins in your DAW. An optional Windows installer is available separately. See `INSTALL.md` for installation/removal and `CHANGELOG.md` for release notes. Third-party dependency notices accompany the plugin.

## Validation and limitations

Automated tests cover mono/stereo at 44.1–192 kHz using six standard rates, variable buffers, zero latency, neutral output, automation, state recall, editor lifecycles and scaled UI rendering. The release QA report in the source repository records exact results.

Native DAW GUI tests, real recorded-vocal listening and hardware per-monitor DPI testing were not performed. Windows x64 VST3 only; no standalone, macOS, VST2 or AU. The installer/plugin are unsigned. Peak protection is sample-based rather than true-peak limiting; saturation is not oversampled. Test levels sensibly on your material.

Source, release downloads and issue reports: https://github.com/scryingorb/YOUR-HALO
