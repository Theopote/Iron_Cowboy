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
    [switch]$HerdIdleSmoke,
    [switch]$IsolationSmoke,
    [switch]$LassoSmoke,
    [switch]$RopeFightSmoke,
    [switch]$CaptureSmoke,
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
if ($HerdIdleSmoke) { if (!$Smoke -or !$Game) { throw 'HerdIdleSmoke requires Game and Smoke.' }; $editorArgs += '-SteppeHerdIdleSmoke' }
if ($IsolationSmoke) { if (!$Smoke -or !$Game) { throw 'IsolationSmoke requires Game and Smoke.' }; $editorArgs += '-SteppeIsolationSmoke' }
if ($LassoSmoke) { if (!$Smoke -or !$Game) { throw 'LassoSmoke requires Game and Smoke.' }; $editorArgs += '-SteppeLassoSmoke' }
if ($RopeFightSmoke) {
    if (!$Smoke -or !$Game) { throw 'RopeFightSmoke requires Game and Smoke.' }
    $editorArgs += '-SteppeLassoSmoke'; $editorArgs += '-SteppeRopeFightSmoke'
}
if ($CaptureSmoke) {
    if (!$Smoke -or !$Game) { throw 'CaptureSmoke requires Game and Smoke.' }
    $editorArgs += '-SteppeLassoSmoke'; $editorArgs += '-SteppeRopeFightSmoke'; $editorArgs += '-SteppeCaptureSmoke'
}
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
    if ($retryLog -match 'STEPPE_P3_SMOKE:') {
        if ($retryLog -notmatch 'STEPPE_P3_SMOKE: Members=5 .*Fleeing=[1-5]') {
            throw "P3 smoke did not confirm a five-horse herd with an active fleeing response; see $logPath"
        }
    }
    elseif ($retryLog -notmatch 'STEPPE_P2_SMOKE: State=EWildHorseState::Fleeing') {
        throw "Chase did not resume after retry; see $logPath"
    }
    Write-Output 'Retry smoke: two reloads reset mounted rider and awareness; chase resumed.'
}
if ($HerdIdleSmoke) {
    $idleLog = Get-Content $logPath -Raw
    if ($idleLog -notmatch 'STEPPE_P3_SMOKE: Members=5 Alert=0 Yielding=0 Fleeing=0 Moving=[1-5] Headings=[2-5] Blocked=0') {
        throw "Idle herd smoke did not confirm varied calm movement without blockage; see $logPath"
    }
    Write-Output 'Idle herd smoke: five calm horses, varied headings, no blockage or flight.'
}
if ($IsolationSmoke) {
    $isolationLog = Get-Content $logPath -Raw
    if ($isolationLog -notmatch 'STEPPE_P4_SMOKE: Focus=H[1-5]') {
        throw "Isolation smoke did not select a visible herd member; see $logPath"
    }
    Write-Output 'Isolation smoke: a visible herd member was selected.'
}
if ($LassoSmoke) {
    $lassoLog = Get-Content $logPath -Raw
    if ($lassoLog -notmatch 'STEPPE_P5_SMOKE: State=ELassoState::Attached .*TargetLassoed=1') {
        throw "Lasso smoke did not attach to and hold the isolated target; see $logPath"
    }
    Write-Output 'Lasso smoke: isolated target was hit and entered the lassoed state.'
}
if ($RopeFightSmoke) {
    $fightLog = Get-Content $logPath -Raw
    if ($fightLog -notmatch 'STEPPE_P6_SMOKE: State=ELassoState::Subdued .*Control=1\.00 Bracing=1') {
        throw "Rope fight smoke did not subdue the target under steady tension; see $logPath"
    }
    Write-Output 'Rope fight smoke: steady bracing subdued the lassoed target.'
}
if ($CaptureSmoke) {
    $captureLog = Get-Content $logPath -Raw
    if ($captureLog -notmatch 'STEPPE_P7_SMOKE: State=ELassoState::Captured Captured=1 Active=4 TargetState=EWildHorseState::Captured') {
        throw "Capture smoke did not register one subdued horse and remove it from the active herd; see $logPath"
    }
    Write-Output 'Capture smoke: one subdued horse was captured and removed from the active herd.'
}
Write-Output "Editor exited successfully. Log: $logPath"
