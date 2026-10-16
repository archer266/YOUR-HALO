param([string]$InstallerPath = '')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
if (!$InstallerPath) { $InstallerPath = Join-Path $projectRoot 'dist/installer/YOUR-HALO-1.0.0-Windows-x64-Setup.exe' }
$InstallerPath = (Resolve-Path -LiteralPath $InstallerPath).Path
$registryPath = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\{EAB8A7D5-98C0-4D80-9F85-53346FC29BB6}_is1'
$machineRegistryPath = $registryPath.Replace('HKCU:', 'HKLM:')
if ((Test-Path -LiteralPath $registryPath) -or (Test-Path -LiteralPath $machineRegistryPath)) { throw 'YOUR HALO is already installer-managed; do not replace its uninstall registration with a smoke test.' }
$smoke = Join-Path $projectRoot ('build/InstallerSmoke-' + [guid]::NewGuid().ToString('N'))
$bundle = Join-Path $smoke 'VST3/VoxxLab/YOUR HALO.vst3'
New-Item -ItemType Directory -Path $smoke -Force | Out-Null
$installArgs = @('/CURRENTUSER', '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-', '/NOICONS',
    ('/DIR="' + $bundle + '"'), ('/LOG="' + (Join-Path $smoke 'install.log') + '"'))
$installed = $false
try {
    $process = Start-Process -FilePath $InstallerPath -ArgumentList $installArgs -WindowStyle Hidden -Wait -PassThru
    if ($process.ExitCode -ne 0) { throw "Installer exit code: $($process.ExitCode)" }
    $installed = $true
    $registration = Get-ItemProperty -LiteralPath $registryPath
    if (![string]::Equals($registration.InstallLocation.TrimEnd('\'), $bundle.TrimEnd('\'), [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Installer did not register the isolated smoke-test path.'
    }
    $source = Join-Path $projectRoot 'build/YOUR_HALO_artefacts/Release/VST3/YOUR HALO.vst3'
    $count = 0
    foreach ($file in Get-ChildItem -LiteralPath $source -File -Recurse) {
        $relative = $file.FullName.Substring($source.Length).TrimStart('\', '/')
        $destination = Join-Path $bundle $relative
        if ((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath $destination).Hash) {
            throw "Installed payload differs: $relative"
        }
        ++$count
    }
    foreach ($required in @('Support/INSTALL.md', 'Support/ThirdPartyLicenses/JUCE-LICENSE.md', 'Support/Uninstall/unins000.exe')) {
        if (!(Test-Path -LiteralPath (Join-Path $bundle $required))) { throw "Missing installer support file: $required" }
    }
    & (Join-Path $projectRoot 'build/Release/HaloTests.exe') $bundle (Join-Path $smoke 'Captures') *> (Join-Path $smoke 'host-tests.log')
    if ($LASTEXITCODE -ne 0) { throw "Installed VST3 regression/host tests failed. See $smoke/host-tests.log" }
    Write-Output "Payload SHA-256 comparison: PASS ($count files). Installed VST3 regression/host tests: PASS."
}
finally {
    if ($installed) {
        $uninstaller = Join-Path $bundle 'Support/Uninstall/unins000.exe'
        if (!(Test-Path -LiteralPath $uninstaller)) { throw "Cannot remove smoke installation automatically: $bundle" }
        $process = Start-Process -FilePath $uninstaller -ArgumentList @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART',
            ('/LOG="' + (Join-Path $smoke 'uninstall.log') + '"')) -WindowStyle Hidden -Wait -PassThru
        if ($process.ExitCode -ne 0) { throw "Uninstaller exit code: $($process.ExitCode)" }
        for ($attempt = 0; $attempt -lt 40 -and (Test-Path -LiteralPath $bundle); ++$attempt) { Start-Sleep -Milliseconds 250 }
        if ((Test-Path -LiteralPath $bundle) -or (Test-Path -LiteralPath $registryPath)) { throw 'Smoke uninstall left plugin files or registration behind.' }
        Write-Output 'Isolated uninstall and registry cleanup: PASS.'
    }
    Write-Output "Smoke-test evidence: $smoke"
}
