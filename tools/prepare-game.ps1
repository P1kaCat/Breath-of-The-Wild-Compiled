param(
    [Parameter(Mandatory)][string]$SourceRoot,
    [Parameter(Mandatory)][string]$CDecryptPath,
    [string]$WorkRoot = (Join-Path (Split-Path -Parent $PSScriptRoot) 'workspace')
)
$ErrorActionPreference = 'Stop'
$SourceRoot = [IO.Path]::GetFullPath($SourceRoot).TrimEnd('\','/')
$WorkRoot = [IO.Path]::GetFullPath($WorkRoot).TrimEnd('\','/')
if ($WorkRoot -eq [IO.Path]::GetPathRoot($WorkRoot).TrimEnd('\','/')) {
    throw 'WorkRoot must be a dedicated project directory, not a drive root.'
}
if ($WorkRoot -eq $SourceRoot -or $WorkRoot.StartsWith($SourceRoot + '\', [StringComparison]::OrdinalIgnoreCase) -or
    $SourceRoot.StartsWith($WorkRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'The source and working copy must use separate directory trees.'
}
if (-not (Test-Path -LiteralPath $CDecryptPath -PathType Leaf)) { throw 'CDecrypt executable not found.' }
$eur = Join-Path $SourceRoot 'DATA\EUR'
$entries = @(
    @{Category='GAMES'; Target='base'; TitlePrefix='00050000'},
    @{Category='UPDATES'; Target='update'; TitlePrefix='0005000E'},
    @{Category='DLCS'; Target='dlc'; TitlePrefix='0005000C'}
)
New-Item -ItemType Directory -Force -Path (Join-Path $WorkRoot 'logs') | Out-Null
foreach ($entry in $entries) {
    # Match literal title IDs rather than interpreting brackets as wildcard ranges.
    $titles = @(Get-ChildItem -LiteralPath (Join-Path $eur $entry.Category) -Directory |
        Where-Object { $_.Name.Contains('[' + $entry.TitlePrefix + '101C9500]') })
    if ($titles.Count -ne 1) { throw "Expected one EUR BOTW title in $($entry.Category); found $($titles.Count)." }
    $snapshot = Join-Path $WorkRoot ('nus-copy\' + $entry.Target)
    $extracted = Join-Path $WorkRoot ('game\' + $entry.Target)
    if (Test-Path -LiteralPath $extracted) { throw "Extraction directory already exists; use a fresh WorkRoot: $extracted" }
    & robocopy $titles[0].FullName $snapshot /E /XJ /COPY:DAT /DCOPY:T /R:1 /W:1 /NP /NFL /NDL /NJH /NJS
    if ($LASTEXITCODE -ge 8) { throw "Copy failed for $($entry.Target)." }
    & $CDecryptPath $snapshot $extracted *> (Join-Path $WorkRoot ('logs\extract-' + $entry.Target + '.log'))
    if ($LASTEXITCODE -ne 0) { throw "Extraction failed for $($entry.Target); see the local extraction log." }
    Write-Host "Copied and extracted: $($entry.Target)"
}
@{ complete=$true; platform='Wii U'; region='EUR'; saves_copied=$false;
    prepared_utc=[DateTime]::UtcNow.ToString('o') } | ConvertTo-Json |
    Set-Content -LiteralPath (Join-Path $WorkRoot 'snapshot.complete.json') -Encoding utf8
Write-Host 'Prepared separate game files. No Cemu saves were copied.'
