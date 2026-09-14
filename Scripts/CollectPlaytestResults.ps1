param(
    [string]$TesterId = '',
    [ValidateRange(1,20)][int]$ExpectedRounds = 3,
    [switch]$AllowPartial
)
$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($TesterId)) { $TesterId = Read-Host 'Enter anonymous TesterId, for example T03' }
if ($TesterId -notmatch '^T[0-9]{2}$') { throw 'TesterId must use T plus two digits, for example T03.' }
$roots = @(
    (Join-Path $PSScriptRoot '..\Saved\Playtests'),
    (Join-Path $PSScriptRoot '..\Steppe\Saved\Playtests'),
    (Join-Path $env:LOCALAPPDATA 'Steppe\Saved\Playtests')
) | Select-Object -Unique
$records = foreach ($root in $roots) {
    if (!(Test-Path -LiteralPath $root)) { continue }
    foreach ($file in Get-ChildItem -LiteralPath $root -Filter 'Round-*.json' -File) {
        try {
            $round = Get-Content -LiteralPath $file.FullName -Raw | ConvertFrom-Json
            if ($round.isAutomated -eq $false) { $file }
        }
        catch { Write-Warning "Skipped unreadable record: $($file.FullName)" }
    }
}
$records = @($records | Sort-Object FullName -Unique | Sort-Object LastWriteTime -Descending | Select-Object -First $ExpectedRounds)
if ($records.Count -eq 0) { throw 'No human playtest records were found. Finish or restart at least one round first.' }
if (!$AllowPartial -and $records.Count -lt $ExpectedRounds) { throw "Found $($records.Count) human rounds; expected $ExpectedRounds. Finish all rounds or pass -AllowPartial." }
$resultRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\PlaytestResults\$TesterId"))
New-Item -ItemType Directory -Path $resultRoot -Force | Out-Null
foreach ($record in $records) { Copy-Item -LiteralPath $record.FullName -Destination (Join-Path $resultRoot $record.Name) -Force }
$zipPath = Join-Path $resultRoot "$TesterId-PlaytestResults.zip"
if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }
Compress-Archive -Path (Join-Path $resultRoot 'Round-*.json') -DestinationPath $zipPath -CompressionLevel Optimal
Write-Output "Collected the latest $($records.Count) human rounds: $zipPath"
