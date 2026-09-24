param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8',
    [string]$Commands = 'QUIT_EDITOR',
    [string]$LogName = 'Editor-Smoke',
    [string]$PythonScript = '',
    [string[]]$ExtraArgs = @(),
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
    [switch]$VerticalSliceSmoke,
    [switch]$VerticalSliceFailureSmoke,
    [switch]$PostCaptureSmoke,
    [switch]$FullLoopSmoke,
    [switch]$ArchetypeSmoke,
    [switch]$LassoSkillSmoke,
    [switch]$BalanceSmoke,
    [switch]$FeedbackSmoke,
    [switch]$PresentationSmoke,
    [switch]$GrasslandSmoke,
    [switch]$MetricsSmoke,
    [switch]$MetricsFailureSmoke,
    [switch]$HorseModelSmoke,
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
if ($VerticalSliceSmoke) {
    if (!$Smoke -or !$Game) { throw 'VerticalSliceSmoke requires Game and Smoke.' }
    $editorArgs += '-SteppeLassoSmoke'; $editorArgs += '-SteppeRopeFightSmoke'; $editorArgs += '-SteppeCaptureSmoke'; $editorArgs += '-SteppeVerticalSliceSmoke'
}
if ($VerticalSliceFailureSmoke) {
    if (!$Smoke -or !$Game) { throw 'VerticalSliceFailureSmoke requires Game and Smoke.' }
    if ($VerticalSliceSmoke) { throw 'Choose either VerticalSliceSmoke or VerticalSliceFailureSmoke.' }
    $editorArgs += '-SteppeVerticalFailureSmoke'
}
if ($PostCaptureSmoke) {
    if (!$Smoke -or !$Game) { throw 'PostCaptureSmoke requires Game and Smoke.' }
    $editorArgs += '-SteppeLassoSmoke'; $editorArgs += '-SteppeRopeFightSmoke'; $editorArgs += '-SteppeCaptureSmoke'; $editorArgs += '-SteppePostCaptureSmoke'
}
if ($FullLoopSmoke) {
    if (!$Smoke -or !$Game) { throw 'FullLoopSmoke requires Game and Smoke.' }
    $editorArgs += '-SteppeLassoSmoke'; $editorArgs += '-SteppeRopeFightSmoke'; $editorArgs += '-SteppeCaptureSmoke'; $editorArgs += '-SteppeFullLoopSmoke'
}
if ($ArchetypeSmoke) {
    if (!$Smoke -or !$Game) { throw 'ArchetypeSmoke requires Game and Smoke.' }
    $editorArgs += '-SteppeArchetypeSmoke'
}
if ($LassoSkillSmoke) {
    if (!$Smoke -or !$Game) { throw 'LassoSkillSmoke requires Game and Smoke.' }
    $editorArgs += '-SteppeLassoSmoke'; $editorArgs += '-SteppeLassoSkillSmoke'
}
if ($BalanceSmoke) {
    if (!$Smoke -or !$Game) { throw 'BalanceSmoke requires Game and Smoke.' }
    $editorArgs += '-SteppeLassoSmoke'; $editorArgs += '-SteppeBalanceSmoke'
}
if ($FeedbackSmoke) {
    if (!$Smoke -or !$Game) { throw 'FeedbackSmoke requires Game and Smoke.' }
    $editorArgs += '-SteppeLassoSmoke'; $editorArgs += '-SteppeLassoSkillSmoke'; $editorArgs += '-SteppeFeedbackSmoke'
}
if ($PresentationSmoke) {
    if (!$Smoke -or !$Game) { throw 'PresentationSmoke requires Game and Smoke.' }
    $editorArgs += '-SteppePresentationSmoke'
}
if ($GrasslandSmoke) {
    if (!$Smoke -or !$Game) { throw 'GrasslandSmoke requires Game and Smoke.' }
    $editorArgs += '-SteppeGrasslandSmoke'
}
if ($MetricsSmoke) {
    if (!$Smoke -or !$Game) { throw 'MetricsSmoke requires Game and Smoke.' }
    if ($MetricsFailureSmoke) { throw 'Choose either MetricsSmoke or MetricsFailureSmoke.' }
    $editorArgs += '-SteppeLassoSmoke'; $editorArgs += '-SteppeRopeFightSmoke'; $editorArgs += '-SteppeCaptureSmoke'; $editorArgs += '-SteppeFullLoopSmoke'; $editorArgs += '-SteppeMetricsSmoke'
}
if ($MetricsFailureSmoke) {
    if (!$Smoke -or !$Game) { throw 'MetricsFailureSmoke requires Game and Smoke.' }
    $editorArgs += '-SteppeVerticalFailureSmoke'; $editorArgs += '-SteppeMetricsSmoke'; $editorArgs += '-SteppeMetricsFailureSmoke'
}
if ($HorseModelSmoke) {
    if (!$Smoke -or !$Game) { throw 'HorseModelSmoke requires Game and Smoke.' }
    $editorArgs += '-SteppeHorseModelSmoke'
}
if ($RetrySmoke) { if (!$Smoke -or !$Game) { throw 'RetrySmoke requires Game and Smoke.' }; $editorArgs += '-SteppeRetrySmoke' }
if ($Tests) { $editorArgs += '-TestExit=Automation Test Queue Empty'; $editorArgs += "-ReportExportPath=$PSScriptRoot\..\Saved\Automation" }
if ($PythonScript) { $editorArgs += "-ExecutePythonScript=$PythonScript" }
elseif (!$Smoke -or $Commands -ne 'QUIT_EDITOR') { $editorArgs += "-ExecCmds=$Commands" }
$editorArgs += $ExtraArgs
$runStarted = Get-Date
& $editorPath @editorArgs
if ($LASTEXITCODE -ne 0) { throw "Editor exited with $LASTEXITCODE; see $logPath" }
if ($PythonScript) {
    $pythonLog = Get-Content $logPath -Raw
    if ($pythonLog -match 'LogPython: Error:|LogEditorPythonExecuter: Error:') { throw "Python asset script reported an error; see $logPath" }
}
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
        if ($retryLog -notmatch 'STEPPE_P3_SMOKE: Members=12 .*Fleeing=(?:[1-9]|1[0-2])') {
            throw "P3 smoke did not confirm a twelve-horse herd with an active fleeing response; see $logPath"
        }
        $coherenceMatch = [regex]::Match($retryLog, 'STEPPE_P3_SMOKE: Members=12 .*Fleeing=12 .*Coherence=([0-9.]+)')
        if (!$coherenceMatch.Success -or [double]$coherenceMatch.Groups[1].Value -lt 0.65) {
            throw "P3 smoke did not confirm coherent group flight; see $logPath"
        }
    }
    elseif ($retryLog -notmatch 'STEPPE_P2_SMOKE: State=EWildHorseState::Fleeing') {
        throw "Chase did not resume after retry; see $logPath"
    }
    Write-Output 'Retry smoke: two reloads reset mounted rider and awareness; chase resumed.'
}
if ($HerdIdleSmoke) {
    $idleLog = Get-Content $logPath -Raw
    if ($idleLog -notmatch 'STEPPE_P3_SMOKE: Members=12 Alert=0 Yielding=0 Fleeing=0 Moving=(?:[8-9]|1[0-2]) Headings=(?:[2-9]|1[0-2]) Blocked=0') {
        throw "Idle herd smoke did not confirm varied calm movement without blockage; see $logPath"
    }
    Write-Output 'Idle herd smoke: twelve calm horses, varied headings, no blockage or flight.'
}
if ($IsolationSmoke) {
    $isolationLog = Get-Content $logPath -Raw
    if ($isolationLog -notmatch 'STEPPE_P4_SMOKE: Focus=H(?:[1-9]|1[0-5])') {
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
if ($VerticalSliceSmoke) {
    $sliceLog = Get-Content $logPath -Raw
    $guidancePath = Join-Path $PSScriptRoot '..\Saved\Screenshots\SteppeP8Guidance.png'
    if (!(Test-Path $guidancePath) -or (Get-Item $guidancePath).LastWriteTime -lt $runStarted) {
        throw "Vertical slice guidance screenshot is missing or stale; see $logPath"
    }
    if ($sliceLog -notmatch 'STEPPE_P8_SMOKE: State=ESteppeTrialState::Success Captured=1 Required=1 .*Score=[1-9][0-9]*') {
        throw "Vertical slice smoke did not complete the timed capture mission with a score; see $logPath"
    }
    Write-Output 'Vertical slice smoke: timed mission completed with one capture and a non-zero score.'
}
if ($VerticalSliceFailureSmoke) {
    $sliceLog = Get-Content $logPath -Raw
    $urgencyPath = Join-Path $PSScriptRoot '..\Saved\Screenshots\SteppeP8Urgency.png'
    if (!(Test-Path $urgencyPath) -or (Get-Item $urgencyPath).LastWriteTime -lt $runStarted) {
        throw "Vertical slice urgency screenshot is missing or stale; see $logPath"
    }
    if ($sliceLog -notmatch 'STEPPE_P8_SMOKE: State=ESteppeTrialState::Failed Captured=0 Required=1 Remaining=0\.0 Score=0') {
        throw "Vertical slice failure smoke did not reach the expected timeout result; see $logPath"
    }
    Write-Output 'Vertical slice failure smoke: urgency warning rendered and uncaptured mission timed out.'
}
if ($PostCaptureSmoke) {
    $sliceLog = Get-Content $logPath -Raw
    $approachPath = Join-Path $PSScriptRoot '..\Saved\Screenshots\SteppeP9Approach.png'
    if (!(Test-Path $approachPath) -or (Get-Item $approachPath).LastWriteTime -lt $runStarted) {
        throw "Post-capture approach screenshot is missing or stale; see $logPath"
    }
    if ($sliceLog -notmatch 'STEPPE_P9_SMOKE: PostState=EPostCaptureState::Leading FirstContact=1 FirstContacts=1 Mounted=0 Calm=1\.00 Trial=ESteppeTrialState::Running') {
        throw "Post-capture smoke did not complete calm first contact while leaving P10 pending; see $logPath"
    }
    Write-Output 'Post-capture smoke: rider dismounted, completed calm approach and left the P10 delivery step pending.'
}
if ($FullLoopSmoke) {
    $loopLog = Get-Content $logPath -Raw
    $leadPath = Join-Path $PSScriptRoot '..\Saved\Screenshots\SteppeP10Lead.png'
    $cardPath = Join-Path $PSScriptRoot '..\Saved\Screenshots\SteppeP10Card.png'
    if (!(Test-Path $leadPath) -or (Get-Item $leadPath).LastWriteTime -lt $runStarted) { throw "P10 lead screenshot is missing or stale; see $logPath" }
    if (!(Test-Path $cardPath) -or (Get-Item $cardPath).LastWriteTime -lt $runStarted) { throw "P10 Horse Card screenshot is missing or stale; see $logPath" }
    if ($loopLog -notmatch 'STEPPE_P10_SMOKE: PostState=EPostCaptureState::Named Leading=0 Delivered=1 Named=1 HorseName=Saran Travel=[3-9][0-9][0-9]\.[0-9] Trial=ESteppeTrialState::Success') {
        throw "P10 smoke did not complete natural lead, delivery and naming; see $logPath"
    }
    Write-Output 'P10 full loop smoke: horse followed through Movement, entered camp, showed its card and was named.'
}
if ($ArchetypeSmoke) {
    $typeLog = Get-Content $logPath -Raw
    $typePath = Join-Path $PSScriptRoot '..\Saved\Screenshots\SteppeP11Archetypes.png'
    if (!(Test-Path $typePath) -or (Get-Item $typePath).LastWriteTime -lt $runStarted) { throw "P11 archetype screenshot is missing or stale; see $logPath" }
    if ($typeLog -notmatch 'STEPPE_P11_SMOKE: H1=Fast .*H2=Strong .*H3=Nervous') {
        throw "P11 smoke did not expose all three archetypes; see $logPath"
    }
    Write-Output 'P11 archetype smoke: Fast, Strong and Nervous profiles rendered and logged.'
}
if ($LassoSkillSmoke) {
    $skillLog = Get-Content $logPath -Raw
    $skillPath = Join-Path $PSScriptRoot '..\Saved\Screenshots\SteppeP12LassoSkill.png'
    $swingPath = Join-Path $PSScriptRoot '..\Saved\Screenshots\SteppeP12Swing.png'
    if (!(Test-Path $swingPath) -or (Get-Item $swingPath).LastWriteTime -lt $runStarted) { throw "P12 swing screenshot is missing or stale; see $logPath" }
    if (!(Test-Path $skillPath) -or (Get-Item $skillPath).LastWriteTime -lt $runStarted) { throw "P12 lasso skill screenshot is missing or stale; see $logPath" }
    if ($skillLog -notmatch 'STEPPE_P12_SMOKE: Stability=(0\.9[0-9]|1\.00) Zone=ELassoHitZone::(Neck|Torso) Radius=[7-9][0-9]\.[0-9] Range=2[5-6][0-9][0-9]\.[0-9]') {
        throw "P12 lasso skill smoke did not confirm a stable physical hit; see $logPath"
    }
    Write-Output 'P12 lasso skill smoke: stable-window throw attached to a physical target volume with expanded loop and range.'
}
if ($BalanceSmoke) {
    $balanceLog = Get-Content $logPath -Raw
    $dragPath = Join-Path $PSScriptRoot '..\Saved\Screenshots\SteppeP12Dragged.png'
    if (!(Test-Path $dragPath) -or (Get-Item $dragPath).LastWriteTime -lt $runStarted) { throw "P12 dragged screenshot is missing or stale; see $logPath" }
    if ($balanceLog -notmatch 'STEPPE_P12_BALANCE_SMOKE: State=ERiderBalanceState::Dragged Mounted=0 Lasso=ELassoState::Attached Balance=1\.00 Side=1\.00') {
        throw "P12 balance smoke did not force a mounted side-load fall into Dragged; see $logPath"
    }
    if ($balanceLog -notmatch 'STEPPE_P12_BALANCE_RELEASE: State=ERiderBalanceState::Recovering Lasso=ELassoState::Recovering') {
        throw "P12 balance smoke did not recover after active rope release; see $logPath"
    }
    Write-Output 'P12 balance smoke: side load forced a fall and short drag; active release started recovery.'
}
if ($FeedbackSmoke) {
    $feedbackLog = Get-Content $logPath -Raw
    $feedbackPath = Join-Path $PSScriptRoot '..\Saved\Screenshots\SteppeP13Feedback.png'
    $hardPath = Join-Path $PSScriptRoot '..\Saved\Screenshots\SteppeP13HardSurface.png'
    if (!(Test-Path $feedbackPath) -or (Get-Item $feedbackPath).LastWriteTime -lt $runStarted) { throw "P13 feedback screenshot is missing or stale; see $logPath" }
    if (!(Test-Path $hardPath) -or (Get-Item $hardPath).LastWriteTime -lt $runStarted) { throw "P13 hard-surface screenshot is missing or stale; see $logPath" }
    if ($feedbackLog -notmatch 'STEPPE_P13_SMOKE: Hoofbeats=[1-9][0-9]* LassoEvents=[2-9][0-9]*? RiskEvents=[0-9]+ Wind=0\.[0-9]+ Breath=0\.[0-9]+ Rope=0\.[0-9]+ Last=') {
        throw "P13 feedback smoke did not produce movement and lasso signals; see $logPath"
    }
    if ($feedbackLog -notmatch 'Surface=ESteppeGroundSurface::Hard') { throw "P13 feedback smoke did not route the hard test pad; see $logPath" }
    Write-Output 'P13 feedback smoke: hoof cadence, movement ambience, rope events and visual feedback rendered.'
}
if ($PresentationSmoke) {
    $presentationLog = Get-Content $logPath -Raw
    $presentationPath = Join-Path $PSScriptRoot '..\Saved\Screenshots\SteppeP13Presentation.png'
    if (!(Test-Path $presentationPath) -or (Get-Item $presentationPath).LastWriteTime -lt $runStarted) { throw "P13 presentation screenshot is missing or stale; see $logPath" }
    if ($presentationLog -notmatch 'STEPPE_P13_PRESENTATION: Gait=EHorseGait::(Gallop|Sprint) Phase=0\.[0-9]+ Stride=0\.[1-9][0-9]* Bob=-?[0-9]+\.[0-9]+ Roll=-?[0-9]+\.[0-9]+ RiderRoll=-?[0-9]+\.[0-9]+ Mounted=1') {
        throw "P13 presentation smoke did not produce a mounted moving pose; see $logPath"
    }
    $legMotion=[regex]::Match($presentationLog,'STEPPE_P16_HORSE_ANIM: BoneDelta=([0-9]+\.[0-9]+) Speed=([0-9]+\.[0-9]+) Clip=HorseGallop')
    if (!$legMotion.Success -or [double]$legMotion.Groups[1].Value -lt 2 -or [double]$legMotion.Groups[2].Value -lt 300) {
        throw "Horse presentation smoke did not confirm skeletal gallop motion on the mounted Blueprint horse; see $logPath"
    }
    Write-Output 'Presentation smoke: mounted gallop rotated the skeletal front leg and rendered rider/horse feedback.'
}
if ($GrasslandSmoke) {
    $grasslandLog = Get-Content $logPath -Raw
    $grasslandPath = Join-Path $PSScriptRoot '..\Saved\Screenshots\SteppeP166Grassland.png'
    if (!(Test-Path $grasslandPath) -or (Get-Item $grasslandPath).LastWriteTime -lt $runStarted) { throw "P16.6 grassland screenshot is missing or stale; see $logPath" }
    if ($grasslandLog -notmatch 'STEPPE_P16_6_GRASSLAND: Landscape=1 RouteActors=2[0-9] Trees=6 Rocks=5 Grass=[1-9][0-9]{3,} Herd=12 HabitatSpread=[1-9][0-9]{2,}') {
        throw "P16.6 grassland smoke did not find the complete gameplay space; see $logPath"
    }
    if ($grasslandLog -notmatch 'STEPPE_P16_6_WATER: Enter=0\.62 Leave=1\.00') { throw "P16.6 shallow-water modifier did not enter and leave cleanly; see $logPath" }
    $terrain = [regex]::Match($grasslandLog,'STEPPE_P16_6_TERRAIN: CampZ=(-?[0-9]+) RidgeZ=(-?[0-9]+) RiverZ=(-?[0-9]+) HardSurface=([0-9]+)')
    if (!$terrain.Success -or [int]$terrain.Groups[2].Value -lt ([int]$terrain.Groups[1].Value + 250) -or [int]$terrain.Groups[3].Value -gt ([int]$terrain.Groups[1].Value - 70) -or [int]$terrain.Groups[4].Value -ne 2) {
        throw "P16.6 ridge, riverbed or hard-ground route did not produce the expected gameplay terrain; see $logPath"
    }
    Write-Output 'P16.6 grassland smoke: Landscape route, habitat, obstacles, camp and shallow-water gameplay validated.'
}
if ($MetricsSmoke -or $MetricsFailureSmoke) {
    $metricsLog = Get-Content $logPath -Raw
    $isFailure = [bool]$MetricsFailureSmoke
    $metricsName = if ($isFailure) { 'P14-Failure.json' } else { 'P14-Success.json' }
    $shotName = if ($isFailure) { 'SteppeP14Failure.png' } else { 'SteppeP14Success.png' }
    $metricsPath = Join-Path $PSScriptRoot "..\Saved\Playtests\$metricsName"
    $shotPath = Join-Path $PSScriptRoot "..\Saved\Screenshots\$shotName"
    if (!(Test-Path $metricsPath) -or (Get-Item $metricsPath).LastWriteTime -lt $runStarted) { throw "P14 metrics JSON is missing or stale; see $logPath" }
    if (!(Test-Path $shotPath) -or (Get-Item $shotPath).LastWriteTime -lt $runStarted) { throw "P14 result screenshot is missing or stale; see $logPath" }
    $metrics = Get-Content $metricsPath -Raw | ConvertFrom-Json
    $expectedResult = if ($isFailure) { 'Failed' } else { 'Success' }
    $expectedReason = if ($isFailure) { 'TimeExpired' } else { 'NamedRequiredHorses' }
    if ($metrics.result -ne $expectedResult -or $metrics.endReason -ne $expectedReason) { throw "P14 metrics result mismatch in $metricsPath" }
    if (!$isFailure -and ($metrics.actions.throws -lt 1 -or $metrics.actions.attachments -lt 1 -or $metrics.stageSeconds.named -lt 0 -or $metrics.score -lt 1)) {
        throw "P14 success metrics did not retain the completed gameplay path; see $metricsPath"
    }
    if ($isFailure -and ($metrics.elapsedSeconds -lt 3 -or $metrics.score -ne 0)) { throw "P14 failure metrics did not retain timeout data; see $metricsPath" }
    if ($metricsLog -notmatch "STEPPE_P14_METRICS: Result=$expectedResult Reason=$expectedReason") { throw "P14 metrics summary is missing; see $logPath" }
    Write-Output "P14 metrics smoke: $expectedResult route wrote a fresh validated JSON record and result screenshot."
}
if ($HorseModelSmoke) {
    $modelLog = Get-Content $logPath -Raw
    $modelPath = Join-Path $PSScriptRoot '..\Saved\Screenshots\SteppeP14HorseModel.png'
    if (!(Test-Path $modelPath) -or (Get-Item $modelPath).LastWriteTime -lt $runStarted) { throw "Horse model screenshot is missing or stale; see $logPath" }
    if ($modelLog -notmatch 'STEPPE_P14_HORSE_MODEL: Parts=16 Legs=4 H1=Fast H2=Strong H3=Nervous') { throw "Horse model smoke did not confirm the complete three-horse assembly; see $logPath" }
    Write-Output 'Horse model smoke: three colored horse silhouettes rendered with body, head, neck, ears, tail and four legs.'
}
Write-Output "Editor exited successfully. Log: $logPath"
