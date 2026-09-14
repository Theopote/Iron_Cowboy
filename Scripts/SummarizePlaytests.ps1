param(
    [string]$PlaytestDirectory = (Join-Path $PSScriptRoot '..\Saved\Playtests'),
    [string]$OutputPath = (Join-Path $PSScriptRoot '..\Saved\Playtests\P14.3-Aggregate.csv'),
    [switch]$IncludeSmoke
)
$ErrorActionPreference = 'Stop'
$files = @(Get-ChildItem -LiteralPath $PlaytestDirectory -Filter '*.json' -File | Sort-Object LastWriteTime)
if ($files.Count -eq 0) { throw "No playtest JSON records in $PlaytestDirectory" }
$rows = foreach ($file in $files) {
    $round = Get-Content -LiteralPath $file.FullName -Raw | ConvertFrom-Json
    if (!$IncludeSmoke -and $round.isAutomated -ne $false) { continue }
    [pscustomobject]@{
        MetricFile = $file.Name
        IsAutomated = [bool]$round.isAutomated
        SessionId = $round.sessionId
        Result = $round.result
        EndReason = $round.endReason
        Archetype = $round.targetArchetype
        HitZone = $round.hitZone
        ElapsedSeconds = [math]::Round([double]$round.elapsedSeconds, 2)
        SelectionSeconds = [math]::Round([double]$round.stageSeconds.targetSelected, 2)
        IsolationSeconds = [math]::Round([double]$round.stageSeconds.isolated, 2)
        CaptureSeconds = [math]::Round([double]$round.stageSeconds.captured, 2)
        ContactSeconds = [math]::Round([double]$round.stageSeconds.firstContact, 2)
        DeliverySeconds = [math]::Round([double]$round.stageSeconds.delivered, 2)
        NamingSeconds = [math]::Round([double]$round.stageSeconds.named, 2)
        Throws = $round.actions.throws
        Attachments = $round.actions.attachments
        Misses = $round.actions.misses
        RopeBreaks = $round.actions.ropeBreaks
        Releases = $round.actions.releases
        DangerousTension = $round.risk.dangerousTensionEntries
        BalanceWarnings = $round.risk.balanceWarnings
        Falls = $round.risk.falls
        Dragged = $round.risk.dragged
        PeakTension = [math]::Round([double]$round.risk.peakTension, 3)
        PeakBalanceRisk = [math]::Round([double]$round.risk.peakBalanceRisk, 3)
        Score = $round.score
    }
}
if (@($rows).Count -eq 0) { throw "No human playtest records with isAutomated=false in $PlaytestDirectory" }
$outputDirectory = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$rows | Export-Csv -LiteralPath $OutputPath -NoTypeInformation -Encoding utf8
Write-Output "Playtest aggregate: $(@($rows).Count) rounds -> $OutputPath"
