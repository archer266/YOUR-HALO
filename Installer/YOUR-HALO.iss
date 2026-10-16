#ifndef HaloVersion
  #define HaloVersion "1.0.0"
#endif
#ifndef HaloLabel
  #define HaloLabel "1.0.0"
#endif
#ifndef BundleDir
  #define BundleDir "..\build\YOUR_HALO_artefacts\Release\VST3\YOUR HALO.vst3"
#endif

[Setup]
AppId={{EAB8A7D5-98C0-4D80-9F85-53346FC29BB6}
AppName=YOUR HALO
AppVersion={#HaloVersion}
AppVerName=YOUR HALO {#HaloLabel}
AppPublisher=VoxxLab
AppPublisherURL=https://github.com/scryingorb/YOUR-HALO
AppSupportURL=https://github.com/scryingorb/YOUR-HALO/issues
AppUpdatesURL=https://github.com/scryingorb/YOUR-HALO/releases
DefaultDirName={commoncf64}\VST3\VoxxLab\YOUR HALO.vst3
DisableDirPage=yes
DisableProgramGroupPage=yes
UsePreviousAppDir=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=commandline
UninstallFilesDir={app}\Support\Uninstall
UninstallDisplayName=YOUR HALO (VoxxLab VST3)
OutputDir=..\dist\installer
OutputBaseFilename=YOUR-HALO-{#HaloLabel}-Windows-x64-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupLogging=yes
CloseApplications=no
RestartApplications=no
VersionInfoVersion={#HaloVersion}
VersionInfoCompany=VoxxLab
VersionInfoDescription=YOUR HALO Windows x64 VST3 installer
VersionInfoProductName=YOUR HALO
VersionInfoProductVersion={#HaloVersion}

[Files]
Source: "{#BundleDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\Docs\Windows-Installation.md"; DestDir: "{app}\Support"; DestName: "INSTALL.md"; Flags: ignoreversion
Source: "..\Docs\ReleaseREADME.md"; DestDir: "{app}\Support"; DestName: "README.md"; Flags: ignoreversion
Source: "..\CHANGELOG.md"; DestDir: "{app}\Support"; Flags: ignoreversion
Source: "..\ThirdPartyLicenses\*"; DestDir: "{app}\Support\ThirdPartyLicenses"; Flags: ignoreversion recursesubdirs createallsubdirs

[Messages]
WelcomeLabel2=This installs the YOUR HALO four-mode vocal enhancer for 64-bit VST3 hosts.%n%nClose your DAW before continuing. The plugin is installed to the standard Windows VST3 folder. No standalone application or desktop shortcut is required.
FinishedLabel=YOUR HALO is installed.%n%nOpen your DAW and rescan VST3 plugins. In FL Studio, use Options > Manage plugins > Find installed plugins, then load YOUR HALO.%n%nHALO controls the amount. AURA, GLOW, SHINE and ASCEND select the processing character.
