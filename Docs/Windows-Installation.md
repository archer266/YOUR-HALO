# Installing YOUR HALO v1.0.0

Requirements: Windows 10 or later, a 64-bit VST3-compatible DAW, and an x64 audio host. YOUR HALO is an audio effect, not a standalone application. Close your DAW before installing, upgrading or removing it.

## Installer

Run `YOUR-HALO-1.0.0-Windows-x64-Setup.exe` and approve the administrator prompt. It installs the complete bundle here:

`C:\Program Files\Common Files\VST3\VoxxLab\YOUR HALO.vst3`

The unsigned installer includes dependency notices and an uninstaller. Remove it using **YOUR HALO (VoxxLab VST3)** in Windows Installed Apps, or `Support\Uninstall\unins000.exe` inside the bundle. Saved DAW project files are not removed.

## Manual ZIP installation

1. Extract `YOUR-HALO-1.0.0-Windows-x64.zip`.
2. Copy the entire `YOUR HALO.vst3` folder into `C:\Program Files\Common Files\VST3\VoxxLab`. Create `VoxxLab` if needed. Administrator permission may be required.
3. Preserve the bundle's `Contents` directory. Do not copy only the inner binary or rename it.
4. Open your DAW and rescan plugins. In FL Studio, use **Options > Manage plugins > Find installed plugins**, then load **YOUR HALO** as a mixer effect.

For manual removal, close your DAW and remove the bundle you copied. Manual ZIP installation does not register an uninstaller.

## Using the plugin

Drag or scroll the HALO orb from 0–100%. Double-click resets it; arrow keys make fine changes. Select AURA for balanced polish, GLOW for body and warmth, SHINE for presence and air, or ASCEND for the strongest density. Both HALO and MODE can be saved and automated. Existing mode selectors are retained; there are no additional controls in this release.

HALO zero is neutral after its 35 ms ramp. Input/output layouts must match: mono or stereo. Tested rates are 44.1, 48, 88.2, 96, 176.4 and 192 kHz. Reported latency is zero. The logo is embedded and requires no external image file or separate runtime download.

Compare the downloaded files against `SHA256SUMS.txt` using `Get-FileHash -Algorithm SHA256`. Host support and known limitations are described in the included README and CHANGELOG.
