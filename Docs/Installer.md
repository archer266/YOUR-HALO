# YOUR HALO Windows installer

YOUR HALO 1.0.0 contains the four-mode vocal-enhancement DSP and fluid pearl/gold orb UI. It is an x64 VST3 plugin for Windows 10 or later, not a standalone application. The installer is unsigned.

## Install and use

1. Close your DAW and run `YOUR-HALO-1.0.0-Windows-x64-Setup.exe`.
2. Allow the administrator prompt. The complete plugin bundle is installed at `C:\Program Files\Common Files\VST3\VoxxLab\YOUR HALO.vst3`.
3. Open your DAW and rescan VST3 plugins. In FL Studio, open **Options > Manage plugins > Find installed plugins**, then load **YOUR HALO** on an audio insert.
4. Turn HALO up from zero and choose **AURA**, **GLOW**, **SHINE** or **ASCEND**. HALO zero is dry after its smoothing ramp. Mono and stereo layouts are supported, and reported latency is zero.

The runtime artwork is embedded; the plugin does not load an image from your desktop. The C++ runtime is linked statically. Dependency notices are included under the installed bundle's `Support/ThirdPartyLicenses` folder.

To remove it, close your DAW and uninstall **YOUR HALO (VoxxLab VST3)** from Windows Installed Apps. Saved DAW project files are not removed. You can also run `Support/Uninstall/unins000.exe` inside the installed bundle. The installer does not add desktop shortcuts or unrelated software.

## Build the installer

Build and test the Release VST3 with `build.ps1`, using JUCE 8.0.12 and the CMake/Visual Studio requirements in the main README. Then install Inno Setup 6.3 or newer and run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build-installer.ps1
# If the compiler is installed elsewhere:
powershell -NoProfile -ExecutionPolicy Bypass -File .\build-installer.ps1 -IsccPath 'C:\path\to\ISCC.exe'
```

The installer and `SHA256SUMS.txt` are written to `dist/installer`. The installer script is `Installer/YOUR-HALO.iss`. Version is checked against CMake; Label controls the downloadable filename. The existing `package.ps1` ZIP workflow and original release reference in `dist` remain separate and are not overwritten by installer packaging.

Installer binaries belong in GitHub Releases, not in normal Git history. Verify the downloaded EXE with `Get-FileHash -Algorithm SHA256` against the published checksum. Signing requires a publisher-owned certificate; this project does not fabricate or bundle one.


## Isolated command-line verification

Run `powershell -NoProfile -ExecutionPolicy Bypass -File .\Installer\Test-Installer.ps1` to install temporarily into a unique `build/InstallerSmoke-*` directory, compare payload hashes, run core/installed-host tests and uninstall. The test refuses to replace an existing installer-managed YOUR HALO registration. It uses `/CURRENTUSER` with an explicit writable `/DIR`, so it exercises a per-user HKCU registration without a UAC prompt. The ordinary installer still defaults to administrator/all-users installation in Common Files. Successful per-user verification does not claim the all-users/UAC flow was tested.
