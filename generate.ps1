param(
    [string]$DolRecompDir = (Join-Path $PSScriptRoot 'third_party\DolRecomp'),
    [string]$UpdateTitleDir = (Join-Path $PSScriptRoot 'game\update'),
    [string]$ToolPath = '',
    [string]$OutputRoot = (Join-Path $PSScriptRoot 'recomp'),
    [int]$Jobs = 12
)

$ErrorActionPreference = 'Stop'
$tool = if ($ToolPath) { $ToolPath } else { Join-Path $DolRecompDir 'build\Release\dolrecomp.exe' }
$rpx = Join-Path $UpdateTitleDir 'code\U-King.rpx'
$output = $OutputRoot
$expectedSha256 = 'BA58DA5B95CE929E005D058CEB08B9B2788D1AB2BBC8A6C189BBADCA0BB34D30'

if (-not (Test-Path -LiteralPath $tool)) {
    throw "DolRecomp is not built: $tool"
}
if (-not (Test-Path -LiteralPath $rpx)) {
    throw "BOTW update RPX is missing: $rpx"
}

$actualSha256 = (Get-FileHash -LiteralPath $rpx -Algorithm SHA256).Hash
if ($actualSha256 -ne $expectedSha256) {
    throw "Unsupported U-King.rpx. Expected BOTW Wii U update v208 SHA-256 $expectedSha256, found $actualSha256"
}

New-Item -ItemType Directory -Force -Path $output | Out-Null
$generationManifest = Join-Path $output 'generation.json'
[IO.File]::WriteAllText($generationManifest, '')
& $tool "-j$Jobs" --cpu espresso --backend c --semantics exact $rpx $output
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& (Join-Path $PSScriptRoot 'tools\patch_generated_hooks.ps1') -GeneratedDir (Join-Path $output 'generated')

$files = Get-ChildItem -LiteralPath (Join-Path $output 'generated') -File -Recurse
$bytes = ($files | Measure-Object Length -Sum).Sum
@{ rpx_sha256=$actualSha256; files=$files.Count; bytes=$bytes; hooks_applied=$true } |
    ConvertTo-Json | Set-Content -LiteralPath $generationManifest -Encoding utf8
Write-Host "Generated $($files.Count) files ($bytes bytes) from BOTW v208."
