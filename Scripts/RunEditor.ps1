param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8',
    [string]$Commands = 'QUIT_EDITOR',
    [string]$LogName = 'Editor-Smoke',
    [string]$PythonScript = '',
    [switch]$Game,
    [switch]$Render,
    [switch]$Tests,
    [switch]$Smoke,
    [switch]$RetrySmoke,
    [ValidateRange(1,10000)][int]$ExpectedTests = 2
)
$ErrorActionPreference = 'Stop'
$projectPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\Steppe.uproject'))
$editorPath = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$logPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\Saved\Logs\$LogName.log"))
$editorArgs = @($projectPath, '-unattended', '-nosplash', '-nosound', "-abslog=$logPath")
if (!$Render) { $editorArgs += '-nullrhi' }
if ($Game) { $editorArgs += '-game' }
if ($Smoke) { $editorArgs += '-SteppeSmoke'; $editorArgs += '-windowed'; $editorArgs += '-ResX=1280'; $editorArgs += '-ResY=720' }
if ($RetrySmoke) { if (!$Smoke -or !$Game) { throw 'RetrySmoke requires Game and Smoke.' }; $editorArgs += '-SteppeRetrySmoke' }
if ($Tests) { $editorArgs += '-TestExit=Automation Test Queue Empty'; $editorArgs += "-ReportExportPath=$PSScriptRoot\..\Saved\Automation" }
if ($PythonScript) { $editorArgs += "-ExecutePythonScript=$PythonScript" }
else { $editorArgs += "-ExecCmds=$Commands" }
$runStarted = Get-Date
& $editorPath @editorArgs
if ($LASTEXITCODE -ne 0) { throw "Editor exited with $LASTEXITCODE; see $logPath" }
if ($Tests) {
    $reportPath = Join-Path $PSScriptRoot '..\Saved\Automation\index.json'
    if ((Get-Item $reportPath).LastWriteTime -lt $runStarted) { throw 'Automation report is stale.' }
    $report = Get-Content $reportPath -Raw | ConvertFrom-Json
    $passed = $report.succeeded + $report.succeededWithWarnings
    if ($report.failed -gt 0 -or $passed -lt $ExpectedTests -or $report.notRun -gt 0 -or $report.inProcess -gt 0) { throw "Automation incomplete/failed: $passed passed (expected at least $ExpectedTests), $($report.failed) failed; see $logPath" }
    Write-Output "Automation: $passed passed, $($report.failed) failed, $($report.succeededWithWarnings) passed with warnings."
}
if ($RetrySmoke) {
    $retryLog = Get-Content $logPath -Raw
    foreach ($reload in 0..2) {
        if ($retryLog -notmatch "STEPPE_RETRY_SMOKE: Reload=$reload Mounted=1 Awareness=0\.00") {
            throw "Retry smoke did not confirm reset $reload; see $logPath"
        }
    }
    if ($retryLog -notmatch 'STEPPE_P2_SMOKE: State=EWildHorseState::Fleeing') {
        throw "Chase did not resume after retry; see $logPath"
    }
    Write-Output 'Retry smoke: two reloads reset mounted rider and awareness; chase resumed.'
}
Write-Output "Editor exited successfully. Log: $logPath"
