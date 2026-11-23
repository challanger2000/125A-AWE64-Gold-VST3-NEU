# Private owner-only test. NO original Creative binary/ROM is uploaded.
param(
    [Parameter(Mandatory=$true)][string]$Bundle,
    [Parameter(Mandatory=$true)][string]$Rom,
    [string]$HostExe = '',
    [int]$Seconds = 45
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if ($env:OS -ne 'Windows_NT') { throw 'Windows x64 required.' }
if ($Seconds -lt 5 -or $Seconds -gt 120) { throw 'Seconds must be 5..120.' }
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$bundlePath = (Resolve-Path $Bundle).Path
$romPath = (Resolve-Path $Rom).Path
if ((Split-Path $bundlePath -Leaf) -ne 'AWE64GoldDOSBoxDev.vst3') {
    throw 'Refusing anything except the exact DOSBox-X development VST3.'
}
$native = Join-Path $bundlePath 'Contents/x86_64-win'
foreach ($filename in @('AWE64GoldDOSBoxDev.vst3','AWE64DOSBoxRuntime.dll')) {
    if (-not (Test-Path -PathType Leaf (Join-Path $native $filename))) {
        throw "Missing real compiled Windows plugin binary: $filename"
    }
}
if ((Get-Item $romPath).Length -ne 1048576) { throw 'Original ROM must be 1 MiB.' }
$expectedRom = '4e143b94f758734f594ded78f4e5115635975c16fa37fabeae0baa4938ca710e'
$expectedSf2 = '78f8739a25d49e519b3fdc92a7f03d4ddd68588fd0260578a880296e304b7b5f'
$expectedDemoBlob = 'e10b8705c6a46444a998c4f6aa74a1cda2081506'
if ((Get-FileHash -Algorithm SHA256 $romPath).Hash.ToLowerInvariant() -ne $expectedRom) {
    throw 'Private ROM SHA-256 mismatch.'
}
$temporary = Join-Path $env:TEMP ('awe64-owner-original-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temporary -Force | Out-Null
try {
    $originalRepo = Join-Path $temporary 'sound-blaster-awe32'
    # Retrieve ORIGINAL public archival Creative developer source to the
    # owner's PRIVATE Windows temp directory; never public GitHub Actions.
    & git clone --depth 1 --branch master --quiet 'https://github.com/qb40/sound-blaster-awe32.git' $originalRepo
    if ($LASTEXITCODE -ne 0) { throw 'Original Creative DOS developer repo unavailable.' }
    $demoRel = 'DOS/DEMO/DEMO.EXE'
    $demo = Join-Path $originalRepo $demoRel
    $sf2 = Join-Path $originalRepo 'DOS/DEMO/SYNTHGM.SF2'
    if (-not (Test-Path -PathType Leaf $demo)) { throw 'Creative DEMO.EXE missing.' }
    if (-not (Test-Path -PathType Leaf $sf2)) { throw 'Creative SYNTHGM.SF2 missing.' }
    $sha = (& git -C $originalRepo hash-object $demoRel).Trim().ToLowerInvariant()
    if ($LASTEXITCODE -ne 0 -or $sha -ne $expectedDemoBlob) {
        throw 'Original Creative DEMO.EXE failed pinned Git-blob verification.'
    }
    if ((Get-Item $sf2).Length -ne 42726 -or
        (Get-FileHash -Algorithm SHA256 $sf2).Hash.ToLowerInvariant() -ne $expectedSf2) {
        throw 'Original SF2 failed pinned size/SHA-256 verification.'
    }
    $privateDir = Join-Path $bundlePath 'Contents/Resources/PRIVATE'
    New-Item -Path $privateDir -ItemType Directory -Force | Out-Null
    $staged = @(
        @{ Source = $romPath; Dest = (Join-Path $privateDir 'awe32.raw') },
        @{ Source = $demo; Dest = (Join-Path $privateDir 'DEMO.EXE') },
        @{ Source = $sf2; Dest = (Join-Path $privateDir 'SYNTHGM.SF2') }
    )
    foreach ($file in $staged) {
        if (Test-Path $file.Dest) {
            # Existing real owner material is NEVER overwritten.
            $old = Get-FileHash -Algorithm SHA256 $file.Dest
            $actual = Get-FileHash -Algorithm SHA256 $file.Source
            if ($old.Hash -ne $actual.Hash) {
                throw ('Refusing to overwrite any existing owner asset: ' + $file.Dest)
            }
        } else {
            Copy-Item -LiteralPath $file.Source -Destination $file.Dest
        }
    }
    Write-Host 'PRIVATE_ORIGINAL_SHA_VALIDATION_PASS; NO_OWNER_BYTES_UPLOADED'
    # SHA-verified original ROM/EXE/SF2 must actually sound through the
    # Steinberg VST3 process() interface, not merely an internal DLL hook.
    $hostTestExe = if ($HostExe) { (Resolve-Path $HostExe).Path } else {
        Join-Path $root 'build-vst3/Release/awe64_vst3_host_pcm_smoke.exe'
    }
    if (-not (Test-Path -PathType Leaf $hostTestExe)) {
        throw 'VST3 test binary absent. Build awe64_vst3_host_pcm_smoke locally or use -HostExe.'
    }
    $pluginBinary = Join-Path $native 'AWE64GoldDOSBoxDev.vst3'
    # Acquire direct mixer evidence before the VST3 gate.
    if ($Seconds -ge 20) {
        & python (Join-Path $root 'tests/WindowsPrivateCreativeGuestSmoke.py') --bundle $bundlePath --seconds $Seconds --reference
        if ($LASTEXITCODE -ne 0) { throw 'Direct DOSBox mixer reference acquisition failed.' }
    }
    $traceFile = Join-Path $temporary 'original_creative_vst3_emu8000.trace'
    # Unique Windows/DOS-compatible 8.3 filename, scoped to this test run.
    # It captures original Creative "ERROR:" messages (detect/init/SF2)
    # from the headless DOS shell, never a test-generated guessed diagnosis.
    $bootFile = 'B' + [guid]::NewGuid().ToString('N').Substring(0,7).ToUpperInvariant() + '.LOG'
    $bootPath = Join-Path $privateDir $bootFile
    if (Test-Path -LiteralPath $bootPath) {
        throw 'Refusing stale original Creative DOS boot diagnostic log.'
    }
    $previousTrace = [Environment]::GetEnvironmentVariable('EMU8K_TRACE', 'Process')
    $previousSdl = [Environment]::GetEnvironmentVariable('SDL_AUDIODRIVER', 'Process')
    $previousOriginal = [Environment]::GetEnvironmentVariable('AWE64_ORIGINAL_CREATIVE_ONLY', 'Process')
    $previousBootLog = [Environment]::GetEnvironmentVariable('AWE64_CREATIVE_BOOT_LOG', 'Process')
    try {
        # Private acceptance: only the Creative/EMU8000 original hardware
        # may supply sound to the VST3. The runtime disables PC-speaker/OPL.
        # SDL2 dummy avoids stealing the physical sound device from the DAW.
        [Environment]::SetEnvironmentVariable('EMU8K_TRACE', $traceFile, 'Process')
        [Environment]::SetEnvironmentVariable('SDL_AUDIODRIVER', 'dummy', 'Process')
        [Environment]::SetEnvironmentVariable('AWE64_ORIGINAL_CREATIVE_ONLY', '1', 'Process')
        [Environment]::SetEnvironmentVariable('AWE64_CREATIVE_BOOT_LOG', $bootFile, 'Process')
        & python (Join-Path $root 'tests/WindowsRunVst3HostPcmSmoke.py') --exe $hostTestExe --plugin $pluginBinary --original --seconds $Seconds
        if ($LASTEXITCODE -ne 0) {
            throw 'Real VST3 process() did not produce MIDI-triggered Creative PCM.'
        }
    } finally {
        # Restore original Windows environment even after timeouts/crashes.
        [Environment]::SetEnvironmentVariable('AWE64_CREATIVE_BOOT_LOG', $previousBootLog, 'Process')
        [Environment]::SetEnvironmentVariable('AWE64_ORIGINAL_CREATIVE_ONLY', $previousOriginal, 'Process')
        [Environment]::SetEnvironmentVariable('SDL_AUDIODRIVER', $previousSdl, 'Process')
        [Environment]::SetEnvironmentVariable('EMU8K_TRACE', $previousTrace, 'Process')
    }
    # Existing original Creative DOS diagnostics may be empty while the
    # interactive demo runs (redirected stdout buffering). The log must
    # at least exist, and explicit startup errors are always fatal.
    if (-not (Test-Path -PathType Leaf $bootPath)) {
        throw 'Original Creative DEMO.EXE startup log was never created.'
    }
    $bootText = [System.IO.File]::ReadAllText($bootPath)
    if ($bootText.ToUpperInvariant().Contains('ERROR:')) {
        # Give the actual original Creative diagnostics, not a generic
        # timeout or a guessed SoundFont/chip failure.
        throw ('Original Creative DOS startup failed: ' +
               $bootText.Substring(0, [Math]::Min(4000, $bootText.Length)))
    }
    if (-not (Test-Path -PathType Leaf $traceFile)) {
        throw 'No original EMU8000 register trace during actual VST3 host playback.'
    }
    $writes = 0
    $ports = [System.Collections.Generic.HashSet[string]]::new()
    foreach ($line in [System.IO.File]::ReadLines($traceFile)) {
        if ($line -match '^\d+ (620|622|A20|A22|E20|E22) [0-9A-Fa-f]{4}$') {
            $writes++
            $null = $ports.Add($Matches[1])
        }
    }
    if ($writes -lt 16 -or $ports.Count -lt 2) {
        throw "Insufficient authentic EMU8000 writes: $writes across $($ports.Count) ports"
    }
    # Independent secondary original-Creative test in an isolated DLL child.
    & python (Join-Path $root 'tests/WindowsPrivateCreativeGuestSmoke.py') --bundle $bundlePath --seconds $Seconds
    if ($LASTEXITCODE -ne 0) { throw 'Private Creative ROM/SF2/DEMO DLL-level PCM gate FAILED.' }
    # Only announce final PASS after BOTH independent tests and all original
    # owner SHA-256/Git-blob and EMU8000 trace gates have actually succeeded.
    Write-Host "PRIVATE_CREATIVE_VST3_EMU8000_PCM_PASS; PORT_WRITES=$writes; ORIGINAL_ROM=VERIFIED; MIDI_NOTE_ON_OFF=VERIFIED"
} finally {
    Remove-Item -LiteralPath $temporary -Recurse -Force -ErrorAction SilentlyContinue
}
