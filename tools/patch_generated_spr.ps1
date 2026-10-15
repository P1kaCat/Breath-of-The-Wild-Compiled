param(
    [Parameter(Mandatory)][string]$GeneratedDir,
    [string[]]$Files = @()
)
$ErrorActionPreference = 'Stop'
$GeneratedDir = [IO.Path]::GetFullPath($GeneratedDir)
$chunks = Join-Path $GeneratedDir 'chunks'
if (-not $Files.Count) {
    $pattern = 'ppc_mtspr\(ctx, (89[6-9]|90[0-3])u|ppc_mfspr\(ctx, (89[6-9]|90[0-3])u'
    $ripgrep = Get-Command rg -ErrorAction SilentlyContinue
    if ($ripgrep) {
        $Files = @(& $ripgrep.Source -l $pattern $chunks)
        if ($LASTEXITCODE -ge 2) { throw 'Could not search generated Espresso SPR instructions.' }
    } else {
        $Files = @(Get-ChildItem -LiteralPath $chunks -File -Filter '*.c' |
            Select-String -Pattern $pattern -List | Select-Object -ExpandProperty Path -Unique)
    }
}
$changed = @()
foreach ($file in $Files) {
    $path = [IO.Path]::GetFullPath($file)
    if (-not $path.StartsWith($chunks + '\',[StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetExtension($path) -ne '.c') {
        throw "Unexpected generated chunk path: $path"
    }
    $source = [IO.File]::ReadAllText($path)
    $write = 'ppc_mtspr\(ctx, (?<spr>\d+)u, ctx->gpr\[(?<reg>\d+)\], 0x(?<cia>[0-9A-Fa-f]{8})u\);\r?\n    if \(ctx->exception\) return;'
    $read = 'ctx->gpr\[(?<reg>\d+)\] = ppc_mfspr\(ctx, (?<spr>\d+)u, 0x(?<cia>[0-9A-Fa-f]{8})u\);\r?\n    if \(ctx->exception\) return;'
    foreach ($operation in @(@{Pattern=$write;Xo=467},@{Pattern=$read;Xo=339})) {
        $source = [regex]::Replace($source,$operation.Pattern,{
            param($match)
            $spr = [int]$match.Groups['spr'].Value
            $reg = [int]$match.Groups['reg'].Value
            if ($spr -lt 896 -or $spr -gt 903) { return $match.Value }
            if ($reg -lt 0 -or $reg -gt 31) { throw 'Invalid SPR register operand.' }
            $raw = (31 -shl 26) -bor ($reg -shl 21) -bor (($spr -band 31) -shl 16) -bor (($spr -shr 5) -shl 11) -bor ($operation.Xo -shl 1)
            $cia = $match.Groups['cia'].Value
            # Preserve the raw instruction and next PC through the existing
            # Espresso handler, then yield to the normal generated dispatcher.
            return ('ppc_fallback_instruction(ctx, 0x{0:X8}u, 0x{1}u);' -f $raw,$cia) + "`n    return;"
        })
    }
    if ($source -ne [IO.File]::ReadAllText($path)) {
        [IO.File]::WriteAllText($path,$source,[Text.UTF8Encoding]::new($false))
        $changed += $path
    }
}
if ($changed.Count) { $changed | Set-Content -LiteralPath (Join-Path $GeneratedDir 'espresso-spr-changes.txt') -Encoding utf8 }
Write-Host "Espresso user quantization: patched $($changed.Count) generated chunks."
exit 0
