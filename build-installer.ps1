param(
    [string]$IsccPath = '',
    [string]$Version = '1.0.0',
    [string]$Label = '1.0.0'
)
$ErrorActionPreference = 'Stop'
if ($Version -notmatch '^\d+\.\d+\.\d+$' -or $Label -notmatch '^[0-9A-Za-z.-]+$') {
    throw 'Version must be major.minor.patch; Label may contain letters, digits, dots and hyphens.'
}
$bundle = Join-Path $PSScriptRoot 'build/YOUR_HALO_artefacts/Release/VST3/YOUR HALO.vst3'
$binary = Join-Path $bundle 'Contents/x86_64-win/YOUR HALO.vst3'
if (!(Test-Path -LiteralPath $binary)) { throw 'Build the Release Windows x64 VST3 before packaging.' }
if (!$IsccPath) {
    $compiler = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($compiler) { $IsccPath = $compiler.Source }
    else {
        foreach ($candidate in @("${env:ProgramFiles(x86)}/Inno Setup 6/ISCC.exe", "$env:ProgramFiles/Inno Setup 6/ISCC.exe")) {
            if (Test-Path -LiteralPath $candidate) { $IsccPath = $candidate; break }
        }
    }
}
if (!$IsccPath -or !(Test-Path -LiteralPath $IsccPath)) { throw 'Install Inno Setup 6.3+ or supply -IsccPath.' }
$project = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'CMakeLists.txt') -Raw
if ($project -notmatch ('project\(YOUR_HALO VERSION ' + [regex]::Escape($Version) + '\s')) {
    throw 'Installer Version must match the CMake project version.'
}
& $IsccPath "/DHaloVersion=$Version" "/DHaloLabel=$Label" "/DBundleDir=$bundle" (Join-Path $PSScriptRoot 'Installer/YOUR-HALO.iss')
if ($LASTEXITCODE -ne 0) { throw 'Installer compilation failed.' }
$output = Join-Path $PSScriptRoot 'dist/installer'
$installer = Join-Path $output "YOUR-HALO-$Label-Windows-x64-Setup.exe"
if (!(Test-Path -LiteralPath $installer)) { throw 'Installer output is missing.' }
$hash = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash.ToLowerInvariant()
$checksum = Join-Path $output 'SHA256SUMS.txt'
[IO.File]::WriteAllText($checksum, "$hash  $([IO.Path]::GetFileName($installer))`n", (New-Object Text.UTF8Encoding($false)))
Write-Output "Installer: $installer"
Write-Output "SHA256: $hash"
