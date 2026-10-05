**Current development build:** Use `build/YOUR_HALO_artefacts/Release/VST3/YOUR HALO.vst3` to audition the four-mode DSP upgrade. The bundle in `dist` remains the original release. In FL Studio, verify HALO and MODE automation, save/reopen both values, switch modes during playback, test mono/stereo vocals, and load an older HALO-only session to confirm AURA. See [the current report](DSPUpgrade/Report.md). Installation still requires copying the complete VST3 bundle with its `Contents` folder.

# Install YOUR HALO in FL Studio on Windows

1. Close FL Studio before installing or replacing the plugin.
2. Extract `YOUR-HALO-1.0.0-Windows-x64.zip`, or use the supplied `dist/YOUR HALO.vst3` folder.
3. Copy the **entire** `YOUR HALO.vst3` folder to `C:\Program Files\Common Files\VST3\VoxxLab\YOUR HALO.vst3`. Create the `VoxxLab` folder if needed. Windows may ask for administrator permission. Keep `Contents/x86_64-win/YOUR HALO.vst3` inside the bundle; do not copy only the inner binary.
4. Open 64-bit FL Studio. Go to **Options → Manage plugins**, enable **Verify plugins**, and choose **Find installed plugins**. If replacing a previous build, enable **Rescan previously verified plugins** as well.
5. Add **YOUR HALO** as an effect in a vocal Mixer insert. Its vendor is **VoxxLab**. Start at 25–50%, then adjust by ear.

The default VST3 location and verified plugin scan follow [Image-Line's installation instructions](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/basics_externalplugins.htm) and [Plugin Manager guidance](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/envsettings_files.htm).

HALO is the only effect parameter. Use FL Studio's **Browse parameters** or **Last tweaked → Create automation clip** to automate it. Host bypass is provided by FL Studio. Double-click the knob to restore zero.

## DAW acceptance checklist

These interactive steps were not performed by the automated host test:

- Scan and load the plugin in FL Studio without an error entry.
- Compare bypass and HALO at 0/25/50/75/100% using recorded vocals at similar perceived volume.
- Try dark, bright and sibilant voices. Check consonants, breath noise and low-level phrases; listen for harshness or pumping.
- Automate fast moves and slow sweeps; record automation from the knob.
- Save an `.flp` with HALO at 37%, close FL Studio, reopen and verify the value and sound.
- Test live input at 64 samples and 44.1/48/88.2/96 kHz where supported by the audio interface.
- Check mono recordings, stereo recordings and mono downmix.
- Close/reopen the editor repeatedly, and test Windows display scaling at 100/150/200%.

For updates, replace the full bundle after closing the DAW, then rescan. To uninstall, remove only `VoxxLab/YOUR HALO.vst3` and rescan.
