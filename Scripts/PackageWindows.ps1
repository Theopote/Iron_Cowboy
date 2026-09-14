param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8',
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\Packages\ProjectSTEPPE-Windows'),
    [switch]$Clean,
    [switch]$Zip
)
$ErrorActionPreference = 'Stop'
$projectPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\Steppe.uproject'))
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$outputPath = [IO.Path]::GetFullPath($OutputDirectory)
$engineVersionPath = Join-Path $EngineRoot 'Engine\Build\Build.version'
$runUatPath = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
if (!(Test-Path -LiteralPath $engineVersionPath)) { throw "Engine version file missing: $engineVersionPath" }
if (!(Test-Path -LiteralPath $runUatPath)) { throw "RunUAT.bat missing: $runUatPath" }
$version = Get-Content -LiteralPath $engineVersionPath -Raw | ConvertFrom-Json
if ($version.MajorVersion -ne 5 -or $version.MinorVersion -ne 8) { throw "STEPPE packaging requires UE 5.8; selected engine is $($version.MajorVersion).$($version.MinorVersion)." }
$pathRoot = [IO.Path]::GetPathRoot($outputPath)
if ($outputPath -eq $projectRoot -or $outputPath -eq $pathRoot -or $outputPath.Length -lt ($pathRoot.Length + 8)) { throw "Unsafe packaging output path: $outputPath" }
if (Test-Path -LiteralPath $outputPath) {
    if (!$Clean) { throw "Output already exists: $outputPath. Pass -Clean to replace it." }
    Remove-Item -LiteralPath $outputPath -Recurse -Force
}
New-Item -ItemType Directory -Path $outputPath -Force | Out-Null
& $runUatPath BuildCookRun "-project=$projectPath" -noP4 -target=Steppe -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -archive "-archivedirectory=$outputPath" "-map=/Game/Steppe/Worlds/Prototype/L_Prototype_Grassland" -prereqs -utf8output
if ($LASTEXITCODE -ne 0) { throw "Windows packaging failed with exit code $LASTEXITCODE" }
$exe = Get-ChildItem -LiteralPath $outputPath -Filter 'Steppe.exe' -File -Recurse | Select-Object -First 1
if (!$exe) { throw "Packaging completed but Steppe.exe was not found in $outputPath" }
$kitPath = Join-Path $outputPath 'PlaytestKit'
$toolsPath = Join-Path $outputPath 'Tools'
New-Item -ItemType Directory -Path $kitPath,$toolsPath -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $projectRoot 'Docs\Playtests\P14.3-PLAYER-GUIDE.md') -Destination $kitPath
Copy-Item -LiteralPath (Join-Path $projectRoot 'Docs\Playtests\P14.3-ORGANIZER-GUIDE.md') -Destination $kitPath
Copy-Item -LiteralPath (Join-Path $projectRoot 'Docs\Playtests\P14.3-Round-Log.csv') -Destination $kitPath
Copy-Item -LiteralPath (Join-Path $projectRoot 'Docs\Playtests\P14.3-Session-Notes.md') -Destination $kitPath
Copy-Item -LiteralPath (Join-Path $projectRoot 'Scripts\CollectPlaytestResults.ps1') -Destination $toolsPath
if ($Zip) {
    $stamp = Get-Date -Format 'yyyyMMdd-HHmm'
    $zipPath = Join-Path (Split-Path -Parent $outputPath) "ProjectSTEPPE-Windows-$stamp.zip"
    if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }
    Compress-Archive -Path (Join-Path $outputPath '*') -DestinationPath $zipPath -CompressionLevel Optimal
    $hash=(Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash
    Set-Content -LiteralPath "$zipPath.sha256.txt" -Value "$hash  $([IO.Path]::GetFileName($zipPath))" -Encoding ascii
    Write-Output "ZIP: $zipPath"
    Write-Output "SHA256: $hash"
}
Write-Output "Windows Shipping package: $outputPath"
Write-Output "Executable: $($exe.FullName)"
