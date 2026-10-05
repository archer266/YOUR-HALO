param(
    [string]$JucePath = '',
    [string]$CMakePath = '',
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release'
)
$ErrorActionPreference = 'Stop'
if (!$CMakePath) {
    $availableCMake = Get-Command cmake -ErrorAction SilentlyContinue
    if ($availableCMake) { $CMakePath = $availableCMake.Source }
    elseif (Test-Path -LiteralPath "$PSScriptRoot/build/CMakeCache.txt") {
        $cachedCommand = Select-String -LiteralPath "$PSScriptRoot/build/CMakeCache.txt" -Pattern '^CMAKE_COMMAND:INTERNAL='
        if ($cachedCommand) { $CMakePath = $cachedCommand.Line.Split('=', 2)[1] }
    }
    if (!$CMakePath -or !(Test-Path -LiteralPath $CMakePath)) { throw 'Pass -CMakePath with a CMake 3.22+ executable, or install CMake on PATH.' }
}
$arguments = @('-S', $PSScriptRoot, '-B', "$PSScriptRoot/build", '-A', 'x64')
if ($JucePath) { $arguments += "-DHALO_JUCE_PATH=$JucePath" }
& $CMakePath @arguments
if ($LASTEXITCODE) { throw 'Configuration failed' }
& $CMakePath --build "$PSScriptRoot/build" --config $Configuration --parallel 2
if ($LASTEXITCODE) { throw 'Build failed' }
& (Join-Path (Split-Path $CMakePath) 'ctest.exe') --test-dir "$PSScriptRoot/build" -C $Configuration --output-on-failure
if ($LASTEXITCODE) { throw 'Tests failed' }
