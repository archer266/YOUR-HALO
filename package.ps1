param([ValidateSet('Debug', 'Release')][string]$Configuration = 'Release')
$ErrorActionPreference = 'Stop'
$bundle = Join-Path $PSScriptRoot "build/YOUR_HALO_artefacts/$Configuration/VST3/YOUR HALO.vst3"
if (!(Test-Path -LiteralPath "$bundle/Contents/x86_64-win/YOUR HALO.vst3")) { throw 'Build the Windows x64 VST3 first.' }
$destination = Join-Path $PSScriptRoot 'dist'
New-Item -ItemType Directory -Path $destination -Force | Out-Null
Copy-Item -LiteralPath $bundle -Destination $destination -Recurse -Force
Copy-Item -LiteralPath "$PSScriptRoot/Docs/FL-Studio.md" -Destination "$destination/INSTALL.md" -Force
Copy-Item -LiteralPath "$PSScriptRoot/ThirdPartyLicenses" -Destination $destination -Recurse -Force
Compress-Archive -LiteralPath "$destination/YOUR HALO.vst3", "$destination/INSTALL.md", "$destination/ThirdPartyLicenses" -DestinationPath "$destination/YOUR-HALO-1.0.0-Windows-x64.zip" -Force
Get-FileHash -LiteralPath "$destination/YOUR HALO.vst3/Contents/x86_64-win/YOUR HALO.vst3" -Algorithm SHA256 |
    Format-List | Out-File -LiteralPath "$destination/SHA256.txt" -Encoding utf8
Write-Output "Bundle and ZIP ready in $destination"
