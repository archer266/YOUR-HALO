param([ValidateSet('Release')][string]$Configuration = 'Release')
$ErrorActionPreference = 'Stop'
$bundle = Join-Path $PSScriptRoot "build/YOUR_HALO_artefacts/$Configuration/VST3/YOUR HALO.vst3"
$binary = Join-Path $bundle 'Contents/x86_64-win/YOUR HALO.vst3'
if (!(Test-Path -LiteralPath $binary)) { throw 'Build and validate the Windows x64 Release VST3 first.' }
$destination = Join-Path $PSScriptRoot 'dist/releases/1.0.0'
$stage = Join-Path $PSScriptRoot ('build/release-stage-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $destination, $stage -Force | Out-Null
Copy-Item -LiteralPath $bundle -Destination $stage -Recurse
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Docs/Windows-Installation.md') -Destination (Join-Path $stage 'INSTALL.md')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Docs/ReleaseREADME.md') -Destination (Join-Path $stage 'README.md')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'CHANGELOG.md') -Destination $stage
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'ThirdPartyLicenses') -Destination $stage -Recurse
$zip = Join-Path $destination 'YOUR-HALO-1.0.0-Windows-x64.zip'
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -Force
# Verify a freshly expanded artifact rather than trusting the staging directory.
$verify = Join-Path $PSScriptRoot ('build/release-verify-' + [guid]::NewGuid().ToString('N'))
Expand-Archive -LiteralPath $zip -DestinationPath $verify
foreach ($file in Get-ChildItem -LiteralPath $stage -File -Recurse) {
    $relative = $file.FullName.Substring($stage.Length).TrimStart('\', '/')
    $unpacked = Join-Path $verify $relative
    if ((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath $unpacked).Hash) {
        throw "ZIP verification failed: $relative"
    }
}
$sourceHash = (Get-FileHash -LiteralPath $binary).Hash
$packagedHash = (Get-FileHash -LiteralPath (Join-Path $verify 'YOUR HALO.vst3/Contents/x86_64-win/YOUR HALO.vst3')).Hash
if ($sourceHash -ne $packagedHash) { throw 'Packaged VST3 does not match the validated build.' }
$zipHash = (Get-FileHash -LiteralPath $zip).Hash.ToLowerInvariant()
[IO.File]::WriteAllText((Join-Path $destination 'SHA256SUMS.txt'), "$zipHash  $([IO.Path]::GetFileName($zip))`n", (New-Object Text.UTF8Encoding($false)))
Write-Output "Verified distribution ZIP: $zip"
Write-Output "Packaged VST3 SHA256: $sourceHash"
Write-Output "ZIP SHA256: $zipHash"
Write-Output "Verified extraction: $verify"
