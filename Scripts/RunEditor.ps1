param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8',
    [string]$Commands = 'QUIT_EDITOR',
    [string]$LogName = 'Editor-Smoke',
    [string]$PythonScript = '',
    [switch]$Game,
    [switch]$Render,
    [switch]$Tests,
    [switch]$Smoke
)
$ErrorActionPreference = 'Stop'
$projectPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\Steppe.uproject'))
$editorPath = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$logPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\Saved\Logs\$LogName.log"))
$editorArgs = @($projectPath, '-unattended', '-nosplash', '-nosound', "-abslog=$logPath")
if (!$Render) { $editorArgs += '-nullrhi' }
if ($Game) { $editorArgs += '-game' }
if ($Smoke) { $editorArgs += '-SteppeSmoke'; $editorArgs += '-windowed'; $editorArgs += '-ResX=1280'; $editorArgs += '-ResY=720' }
if ($Tests) { $editorArgs += '-TestExit=Automation Test Queue Empty'; $editorArgs += "-ReportExportPath=$PSScriptRoot\..\Saved\Automation" }
if ($PythonScript) { $editorArgs += "-ExecutePythonScript=$PythonScript" }
else { $editorArgs += "-ExecCmds=$Commands" }
& $editorPath @editorArgs
if ($LASTEXITCODE -ne 0) { throw "Editor exited with $LASTEXITCODE; see $logPath" }
if ($Tests) {
    $report = Get-Content (Join-Path $PSScriptRoot '..\Saved\Automation\index.json') -Raw | ConvertFrom-Json
    if ($report.failed -gt 0 -or $report.succeeded -lt 2) { throw "Automation incomplete/failed: $($report.succeeded) succeeded, $($report.failed) failed; see $logPath" }
}
Write-Output "Editor exited successfully. Log: $logPath"
