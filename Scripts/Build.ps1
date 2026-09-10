param(
    [Parameter(Mandatory = $true)][string]$EngineRoot,
    [ValidateSet('Development', 'DebugGame')][string]$Configuration = 'Development'
)
$ErrorActionPreference = 'Stop'
$versionPath = Join-Path $EngineRoot 'Engine\Build\Build.version'
if (!(Test-Path -LiteralPath $versionPath)) { throw "Engine version file missing: $versionPath" }
$version = Get-Content -LiteralPath $versionPath -Raw | ConvertFrom-Json
if ($version.MajorVersion -ne 5 -or $version.MinorVersion -ne 8) {
    throw "STEPPE requires UE 5.8. Selected engine is $($version.MajorVersion).$($version.MinorVersion). No build was started."
}
$projectPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\Steppe.uproject'))
$buildPath = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
if (!(Test-Path -LiteralPath $buildPath)) { throw "Build.bat missing: $buildPath" }
& $buildPath SteppeEditor Win64 $Configuration "-Project=$projectPath" -WaitMutex -NoUBA
if ($LASTEXITCODE -ne 0) { throw "Unreal build failed with exit code $LASTEXITCODE" }
