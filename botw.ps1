param(
    [ValidateSet('doctor','components','translator','generate','build','test','run')]
    [string]$Action = 'doctor',
    [string]$WorkRoot = (Join-Path $PSScriptRoot 'workspace'),
    [string]$BuildRoot = '',
    [string]$PrecompiledObjects = '',
    [ValidateSet('Auto','Visual Studio 17 2022','Ninja')]
    [string]$Generator = 'Auto',
    [switch]$RecompileGenerated,
    [ValidateRange(1,16)][int]$Jobs = 4,
    [ValidateRange(320,7680)][int]$Width = 1280,
    [ValidateRange(180,4320)][int]$Height = 720,
    [ValidateSet('nearest','linear')][string]$PresentationFilter = 'linear',
    [switch]$VSync,
    [ValidateRange(0,2147483647)][int]$Blocks = 10000,
    [ValidateRange(0,600)][int]$Seconds = 0,
    [switch]$Headless,
    [switch]$Mute,
    [switch]$Capture,
    [switch]$ShaderOverrides
)

$ErrorActionPreference = 'Stop'
$WorkRoot = [IO.Path]::GetFullPath($WorkRoot)
if ($WorkRoot.TrimEnd('\','/') -eq [IO.Path]::GetPathRoot($WorkRoot).TrimEnd('\','/')) {
    throw 'WorkRoot must be a dedicated project directory, not a drive root.'
}
$base = Join-Path $WorkRoot 'game\base'
$update = Join-Path $WorkRoot 'game\update'
$dlc = Join-Path $WorkRoot 'game\dlc'
$generated = Join-Path $WorkRoot 'recomp\generated'
$translator = Join-Path $WorkRoot 'translator\dolrecomp\Release\dolrecomp.exe'
if (-not (Test-Path -LiteralPath $translator)) {
    $translator = Join-Path $WorkRoot 'translator\dolrecomp\dolrecomp.exe'
}
$buildStatePath = Join-Path $WorkRoot 'native-build.json'
if (-not $BuildRoot -and (Test-Path -LiteralPath $buildStatePath)) {
    $buildState = Get-Content -LiteralPath $buildStatePath -Raw | ConvertFrom-Json
    $BuildRoot = $buildState.build_root
}
$gameBuild = if ($BuildRoot) { [IO.Path]::GetFullPath($BuildRoot) } else { Join-Path $WorkRoot 'game-build' }
$componentBuild = Join-Path $WorkRoot 'components'
$manifest = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'project.json') -Raw | ConvertFrom-Json

function Invoke-Checked([string]$Executable, [string[]]$Arguments) {
    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Executable failed (exit $LASTEXITCODE)." }
}
function Assert-GameInputs {
    foreach ($directory in @((Join-Path $base 'content'), (Join-Path $update 'content'))) {
        if (-not (Test-Path -LiteralPath $directory -PathType Container)) {
            throw "Extracted game snapshot missing: $directory"
        }
    }
    $rpx = Join-Path $update 'code\U-King.rpx'
    if (-not (Test-Path -LiteralPath $rpx -PathType Leaf)) { throw "RPX missing: $rpx" }
    $hash = (Get-FileHash -LiteralPath $rpx -Algorithm SHA256).Hash
    if ($hash -ne $manifest.supported_rpx_sha256) {
        throw "RPX is not supported by the inherited v208 hooks. Found SHA-256 $hash. Region/version adaptation is required."
    }
    Write-Host "RPX verified: $hash"
}
function Configure-Host([string]$Source, [string]$Build, [string[]]$Extra) {
    $cache = Join-Path $Build 'CMakeCache.txt'
    $selected = $Generator
    if (Test-Path -LiteralPath $cache) {
        $entry = Select-String -LiteralPath $cache -Pattern '^CMAKE_GENERATOR:INTERNAL=(.+)$' | Select-Object -First 1
        if ($entry) {
            $existing = $entry.Matches[0].Groups[1].Value
            if ($Generator -ne 'Auto' -and $Generator -ne $existing) {
                throw 'This build directory uses another generator. Select a fresh BuildRoot.'
            }
            $selected = $existing
        }
    }
    if ($selected -eq 'Auto') { $selected = 'Visual Studio 17 2022' }
    if ($selected -eq 'Ninja') {
        Invoke-Checked 'cmake' (@('-S',$Source,'-B',$Build,'-G','Ninja','-DCMAKE_BUILD_TYPE=Release') + $Extra)
    } else {
        $toolset = if (Test-Path -LiteralPath $cache) { @() } else { @('-T','host=x64') }
        Invoke-Checked 'cmake' (@('-S',$Source,'-B',$Build,'-G',$selected,'-A','x64') + $toolset + $Extra)
    }
}
function Build-Host([string]$Build, [string[]]$Extra = @()) {
    $native = if (Select-String -LiteralPath (Join-Path $Build 'CMakeCache.txt') -Pattern '^CMAKE_GENERATOR:INTERNAL=Visual Studio ' -Quiet) {
        @('--','/p:PreferredToolArchitecture=x64')
    } else { @() }
    Invoke-Checked 'cmake' (@('--build',$Build,'--config','Release','--parallel',"$Jobs") + $Extra + $native)
}

if ($Action -eq 'doctor') {
    Write-Host "BOTW Compiled: experimental Wii U port; backend D3D11."
    foreach ($name in @('cmake','ctest')) {
        $command = Get-Command $name -ErrorAction SilentlyContinue
        if ($command) { Write-Host "$name : $($command.Source)" }
        else { Write-Host "$name : MISSING" }
    }
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $cppInstallation = if (Test-Path -LiteralPath $vswhere) {
        & $vswhere -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    } else { '' }
    if ($cppInstallation) { Write-Host "Visual Studio 2022 C++ : $cppInstallation" }
    else { Write-Host 'Visual Studio 2022 C++ installation : MISSING. Ninja can also use an initialized MSVC build environment.' }
    $compiler = Get-Command cl -ErrorAction SilentlyContinue
    if ($compiler) { Write-Host "MSVC build environment : $($compiler.Source)" }
    Write-Host "Builds / copied game data: $WorkRoot"
    Write-Host "Native build: $gameBuild"
    Write-Host "Translator built: $(Test-Path -LiteralPath $translator)"
    Write-Host "Game executable built: $(Test-Path -LiteralPath (Join-Path $gameBuild 'bin\botw_recomp.exe'))"
    Assert-GameInputs
    Write-Host 'RPX and content directories verified. This does not confirm complete extraction or playable rendering.'
    return
}

New-Item -ItemType Directory -Force -Path (Join-Path $WorkRoot 'temp') | Out-Null
$previousTemp = $env:TEMP
$previousTmp = $env:TMP
$env:TEMP = Join-Path $WorkRoot 'temp'
$env:TMP = $env:TEMP
try {
    switch ($Action) {
        'components' {
            Configure-Host $PSScriptRoot $componentBuild @('-DBOTW_BUILD_GAME=OFF')
            Build-Host $componentBuild
            Invoke-Checked 'ctest' @('--test-dir',$componentBuild,'-C','Release','--output-on-failure')
        }
        'translator' {
            $build = Join-Path $WorkRoot 'translator'
            Configure-Host (Join-Path $PSScriptRoot 'cmake\Translator') $build @()
            Build-Host $build @('--target','dolrecomp')
        }
        'generate' {
            Assert-GameInputs
            if (-not (Test-Path -LiteralPath $translator)) { throw 'Run .\botw.ps1 translator first.' }
            & (Join-Path $PSScriptRoot 'generate.ps1') -UpdateTitleDir $update -ToolPath $translator -OutputRoot (Join-Path $WorkRoot 'recomp') -Jobs $Jobs
            if ($LASTEXITCODE -ne 0) { throw "Generation failed (exit $LASTEXITCODE)." }
        }
        'build' {
            Assert-GameInputs
            [xml]$metadata = Get-Content -LiteralPath (Join-Path $update 'meta\meta.xml') -Raw
            $titleId = [string]$metadata.menu.title_id.'#text'
            $groupId = [string]$metadata.menu.group_id.'#text'
            $region = [string]$metadata.menu.region.'#text'
            if ($titleId -notmatch '^[0-9A-Fa-f]{16}$' -or $groupId -notmatch '^[0-9A-Fa-f]{8}$' -or $region -notmatch '^[0-9A-Fa-f]{8}$') {
                throw 'Invalid title identity fields in copied meta.xml.'
            }
            $generationPath = Join-Path $WorkRoot 'recomp\generation.json'
            if (-not (Test-Path -LiteralPath $generationPath)) { throw 'No completed generation. Run generate first.' }
            $generation = Get-Content -LiteralPath $generationPath -Raw | ConvertFrom-Json
            if (-not $generation -or $generation.rpx_sha256 -ne $manifest.supported_rpx_sha256) {
                throw 'Generated sources are incomplete or from an unsupported RPX.'
            }
            $objectCache = if ($RecompileGenerated) { @('-DBOTW_PRECOMPILED_OBJECT_DIR=') } elseif ($PrecompiledObjects) { @("-DBOTW_PRECOMPILED_OBJECT_DIR=$PrecompiledObjects") } else { @() }
            Configure-Host $PSScriptRoot $gameBuild (@('-DBOTW_BUILD_GAME=ON',"-DBOTW_COMPILE_JOBS=$Jobs","-DGENERATED_DIR=$generated","-DBOTW_BASE_TITLE_DIR=$base","-DBOTW_UPDATE_TITLE_DIR=$update","-DBOTW_DLC_TITLE_DIR=$dlc","-DBOTW_GAME_TITLE_ID=$titleId","-DBOTW_GAME_GROUP_ID=$groupId","-DBOTW_GAME_REGION=$region") + $objectCache)
            Build-Host $gameBuild
            @{ build_root=$gameBuild } | ConvertTo-Json | Set-Content -LiteralPath $buildStatePath -Encoding utf8
        }
        'test' {
            $testBuild = if (Test-Path -LiteralPath (Join-Path $gameBuild 'bin\botw_recomp.exe')) { $gameBuild } else { $componentBuild }
            Invoke-Checked 'ctest' @('--test-dir',$testBuild,'-C','Release','--output-on-failure')
        }
        'run' {
            Assert-GameInputs
            $snapshotPath = Join-Path $WorkRoot 'snapshot.complete.json'
            if (-not (Test-Path -LiteralPath $snapshotPath)) { throw 'Game extraction is not marked complete. Finish preparation before running.' }
            $snapshot = Get-Content -LiteralPath $snapshotPath -Raw | ConvertFrom-Json
            if (-not $snapshot.complete) { throw 'Game extraction is incomplete.' }
            $executable = Join-Path $gameBuild 'bin\botw_recomp.exe'
            if (-not (Test-Path -LiteralPath $executable)) { throw 'Run generate and build first.' }
            $settings = @{
                BOTW_WINDOW_WIDTH="$Width"; BOTW_WINDOW_HEIGHT="$Height";
                BOTW_VSYNC=$(if ($VSync) {'1'} else {'0'});
                BOTW_PRESENT_FILTER=$PresentationFilter;
                BOTW_HEADLESS=$(if ($Headless) {'1'} else {'0'});
                BOTW_MUTE=$(if ($Mute) {'1'} else {'0'});
                BOTW_WIIU_MLC_ROOT=(Join-Path $WorkRoot 'system\mlc01');
                BOTW_CAPTURE_DIR=$(if ($Capture) { Join-Path $WorkRoot 'captures' } else { $null });
                BOTW_SHADER_OVERRIDES=$(if ($ShaderOverrides) { Join-Path $WorkRoot 'shader-overrides' } else { $null });
                BOTW_LOG_PATH=(Join-Path $WorkRoot 'logs\runtime.log')
            }
            $previous = @{}
            foreach ($key in $settings.Keys) {
                $previous[$key] = [Environment]::GetEnvironmentVariable($key, 'Process')
                [Environment]::SetEnvironmentVariable($key, $settings[$key], 'Process')
            }
            New-Item -ItemType Directory -Force -Path (Join-Path $WorkRoot 'run'), (Join-Path $WorkRoot 'logs'), (Join-Path $WorkRoot 'system\mlc01') | Out-Null
            if ($Capture) { New-Item -ItemType Directory -Force -Path (Join-Path $WorkRoot 'captures') | Out-Null }
            if ($ShaderOverrides) { New-Item -ItemType Directory -Force -Path (Join-Path $WorkRoot 'shader-overrides') | Out-Null }
            Push-Location (Join-Path $WorkRoot 'run')
            try {
                $arguments = @('--base',$base,'--update',$update,'--dlc',$dlc,'--blocks',"$Blocks")
                if ($Seconds -gt 0) {
                    $start = [Diagnostics.ProcessStartInfo]::new()
                    $start.FileName = $executable
                    $start.UseShellExecute = $false
                    $start.CreateNoWindow = $true
                    $start.WindowStyle = [Diagnostics.ProcessWindowStyle]::Hidden
                    $start.WorkingDirectory = (Get-Location).Path
                    $start.RedirectStandardOutput = $true
                    foreach ($argument in $arguments) { $start.ArgumentList.Add($argument) }
                    $process = [Diagnostics.Process]::Start($start)
                    $output = $process.StandardOutput.ReadToEndAsync()
                    Write-Host "Diagnostic process PID=$($process.Id), limit=$Seconds seconds."
                    $timer = [Diagnostics.Stopwatch]::StartNew()
                    $timedOut = $false
                    while (-not $process.WaitForExit(1000)) {
                        if ($timer.Elapsed.TotalSeconds -ge $Seconds) {
                            $process.Kill()
                            $process.WaitForExit()
                            $timedOut = $true
                            Write-Host "Stopped this diagnostic process after $Seconds seconds."
                            break
                        }
                    }
                    $output.GetAwaiter().GetResult() | Set-Content -LiteralPath (Join-Path $WorkRoot 'logs\runtime-stdout.log') -Encoding utf8
                    $exitCode = $process.ExitCode
                    @{ process_id=$process.Id; elapsed_seconds=$timer.Elapsed.TotalSeconds;
                        timed_out=$timedOut; exit_code=$exitCode } | ConvertTo-Json |
                        Set-Content -LiteralPath (Join-Path $WorkRoot 'logs\runtime-probe.json') -Encoding utf8
                    $process.Dispose()
                    if (-not $timedOut -and $exitCode -ne 0) { throw "Runtime exited with code $exitCode; see runtime.log." }
                } else {
                    Invoke-Checked $executable $arguments
                }
            } finally {
                Pop-Location
                foreach ($key in $settings.Keys) { [Environment]::SetEnvironmentVariable($key, $previous[$key], 'Process') }
            }
        }
    }
} finally {
    $env:TEMP = $previousTemp
    $env:TMP = $previousTmp
}
