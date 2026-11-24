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
    # Independently run the public author-bank regression before other
    # original gates can fail. No owner data or downloaded assets uploaded.
    $cheezExit = 0
    if ($env:AWE64_TEST_CHEEZOIDS -eq '1') {
        & python (Join-Path $root 'tests/WindowsCheezoidsSweep.py') --bundle $bundlePath --exe $hostTestExe
        $cheezExit = $LASTEXITCODE
    }
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
    # Test a separately selected reference SF2 through the actual serialized
    # IComponent state and the production external-bank DOS mount path.
    # Never change the owner's source, default bank or the first trace.
    $external = Join-Path $temporary 'external reference.sf2'
    Copy-Item -LiteralPath $sf2 -Destination $external -ErrorAction Stop
    $externalHash = (Get-FileHash -Algorithm SHA256 $external).Hash
    $externalBoot = 'B' + [guid]::NewGuid().ToString('N').Substring(0,7).ToUpperInvariant() + '.LOG'
    $externalTrace = Join-Path $temporary 'external_creative_vst3_emu8000.trace'
    $previousExternal = [Environment]::GetEnvironmentVariable('AWE64_TEST_EXTERNAL_SF2', 'Process')
    try {
        [Environment]::SetEnvironmentVariable('AWE64_TEST_EXTERNAL_SF2', $external, 'Process')
        [Environment]::SetEnvironmentVariable('EMU8K_TRACE', $externalTrace, 'Process')
        [Environment]::SetEnvironmentVariable('SDL_AUDIODRIVER', 'dummy', 'Process')
        [Environment]::SetEnvironmentVariable('AWE64_ORIGINAL_CREATIVE_ONLY', '1', 'Process')
        [Environment]::SetEnvironmentVariable('AWE64_CREATIVE_BOOT_LOG', $externalBoot, 'Process')
        & python (Join-Path $root 'tests/WindowsRunVst3HostPcmSmoke.py') --exe $hostTestExe --plugin $pluginBinary --original --seconds $Seconds
        if ($LASTEXITCODE -ne 0) { throw 'External reference SF2 real VST3 gate FAILED.' }
        if ((Get-FileHash -Algorithm SHA256 $external).Hash -ne $externalHash) {
            throw 'External source was unexpectedly modified.'
        }
        if (-not (Test-Path -PathType Leaf $externalTrace)) { throw 'External SF2 EMU8000 trace missing.' }
        $externalWrites = 0
        foreach ($line in [System.IO.File]::ReadLines($externalTrace)) {
            if ($line -match '^\d+ (620|622|A20|A22|E20|E22) [0-9A-Fa-f]{4}$') { $externalWrites++ }
        }
        if ($externalWrites -lt 16) { throw 'External SF2 original EMU8000 register trace insufficient.' }
        Write-Host "EXTERNAL_REFERENCE_SF2_STATE_AND_ORIGINAL_PCM_PASS; PORT_WRITES=$externalWrites; SOURCE_UNCHANGED"
    } finally {
        [Environment]::SetEnvironmentVariable('AWE64_TEST_EXTERNAL_SF2', $previousExternal, 'Process')
        [Environment]::SetEnvironmentVariable('AWE64_CREATIVE_BOOT_LOG', $previousBootLog, 'Process')
        [Environment]::SetEnvironmentVariable('AWE64_ORIGINAL_CREATIVE_ONLY', $previousOriginal, 'Process')
        [Environment]::SetEnvironmentVariable('SDL_AUDIODRIVER', $previousSdl, 'Process')
        [Environment]::SetEnvironmentVariable('EMU8K_TRACE', $previousTrace, 'Process')
    }
    # Newly generated PCM, not ROM-derived. Verify two sparse program IDs
    # by distinct audible frequencies, not merely nonzero output.
    $ramFixture = Join-Path $temporary '125A RAM probe.sf2'
    & python (Join-Path $root 'tests/MakeRamSampleSf2.py') --output $ramFixture
    if ($LASTEXITCODE -ne 0) { throw 'RAM fixture creation failed.' }
    $ramHash = (Get-FileHash -Algorithm SHA256 $ramFixture).Hash
    $ramFailures = @()
    # Measure the same self-authored SF2 in the original DLL/Creative guest
    # directly, without IComponent, state handling or VST3 PCM conversion.
    foreach ($program in @(0,17)) {
        & python (Join-Path $root 'tests/WindowsPrivateCreativeGuestSmoke.py') --bundle $bundlePath --seconds $Seconds --reference --ram-program $program
        if ($LASTEXITCODE -ne 0) { $ramFailures += "direct RAM program $program" }
    }
    $previousRamHz = [Environment]::GetEnvironmentVariable('AWE64_TEST_RAM_TONE_HZ','Process')
    $previousHostRate = [Environment]::GetEnvironmentVariable('AWE64_TEST_HOST_RATE','Process')
    try {
        foreach ($case in @('441:44100','735:44100','735:48000')) {
            $hz,$rate = $case.Split(':')
            [Environment]::SetEnvironmentVariable('AWE64_TEST_HOST_RATE', $rate, 'Process')
            $ramBoot = 'B' + [guid]::NewGuid().ToString('N').Substring(0,7).ToUpperInvariant() + '.LOG'
            $ramTrace = Join-Path $temporary ("ram_" + $hz + "_" + $rate + ".trace")
            [Environment]::SetEnvironmentVariable('AWE64_TEST_EXTERNAL_SF2', $ramFixture, 'Process')
            [Environment]::SetEnvironmentVariable('AWE64_TEST_RAM_TONE_HZ', $hz, 'Process')
            [Environment]::SetEnvironmentVariable('EMU8K_TRACE', $ramTrace, 'Process')
            [Environment]::SetEnvironmentVariable('SDL_AUDIODRIVER', 'dummy', 'Process')
            [Environment]::SetEnvironmentVariable('AWE64_ORIGINAL_CREATIVE_ONLY', '1', 'Process')
            [Environment]::SetEnvironmentVariable('AWE64_CREATIVE_BOOT_LOG', $ramBoot, 'Process')
            & python (Join-Path $root 'tests/WindowsRunVst3HostPcmSmoke.py') --exe $hostTestExe --plugin $pluginBinary --original --seconds $Seconds
            $ramExit = $LASTEXITCODE
            if (Test-Path -PathType Leaf $ramTrace) {
                & python (Join-Path $root 'tests/WindowsPrivateCreativeGuestSmoke.py') --trace-summary $ramTrace
                if ($LASTEXITCODE -ne 0) { $ramFailures += "trace summary ${hz}:${rate}" }
            } else { $ramFailures += "missing trace ${hz}:${rate}" }
            if ($ramExit -ne 0) {
                $ramFailures += "VST3 RAM spectral ${hz}:${rate}"
                continue
            }
            if ((Get-FileHash -Algorithm SHA256 $ramFixture).Hash -ne $ramHash) { throw 'RAM fixture modified.' }
            Write-Host "RAM_SAMPLE_PROGRAM_SPECTRAL_PASS; HZ=$hz; HOST_RATE=$rate; SOURCE_UNCHANGED"
        }
    } finally {
        [Environment]::SetEnvironmentVariable('AWE64_TEST_RAM_TONE_HZ', $previousRamHz, 'Process')
        [Environment]::SetEnvironmentVariable('AWE64_TEST_HOST_RATE', $previousHostRate, 'Process')
        [Environment]::SetEnvironmentVariable('AWE64_TEST_EXTERNAL_SF2', $previousExternal, 'Process')
        [Environment]::SetEnvironmentVariable('AWE64_CREATIVE_BOOT_LOG', $previousBootLog, 'Process')
        [Environment]::SetEnvironmentVariable('AWE64_ORIGINAL_CREATIVE_ONLY', $previousOriginal, 'Process')
        [Environment]::SetEnvironmentVariable('SDL_AUDIODRIVER', $previousSdl, 'Process')
        [Environment]::SetEnvironmentVariable('EMU8K_TRACE', $previousTrace, 'Process')
    }
    # Isolate absolute send endpoint semantics before enabling GUI controls.
    $sendFailures = @()
    foreach ($cc in @(0,127)) {
        & python (Join-Path $root 'tests/WindowsPrivateCreativeGuestSmoke.py') --bundle $bundlePath --seconds $Seconds --reference --ram-program 0 --send-reference $cc
        if ($LASTEXITCODE -ne 0) { $sendFailures += "CC91/93=$cc" }
    }
    # Product absolute sends: adversarial embedded wet generators, both
    # endpoints independently, plus actual native controller/state roundtrip.
    $wetFixture = Join-Path $temporary '125A wet send probe.sf2'
    & python (Join-Path $root 'tests/MakeRamSampleSf2.py') --output $wetFixture --wet
    if ($LASTEXITCODE -ne 0) { throw 'Wet fixture creation failed.' }
    $wetHash = (Get-FileHash -Algorithm SHA256 $wetFixture).Hash
    $previousPair = [Environment]::GetEnvironmentVariable('AWE64_TEST_SEND_PAIR','Process')
    try {
        foreach ($pair in @(0,255,65280)) {
            $sendTrace = Join-Path $temporary ("send_" + $pair + ".trace")
            $sendBoot = 'B' + [guid]::NewGuid().ToString('N').Substring(0,7).ToUpperInvariant() + '.LOG'
            [Environment]::SetEnvironmentVariable('AWE64_TEST_SEND_PAIR', [string]$pair, 'Process')
            [Environment]::SetEnvironmentVariable('AWE64_TEST_EXTERNAL_SF2', $wetFixture, 'Process')
            [Environment]::SetEnvironmentVariable('AWE64_TEST_RAM_TONE_HZ', '441', 'Process')
            [Environment]::SetEnvironmentVariable('EMU8K_TRACE', $sendTrace, 'Process')
            [Environment]::SetEnvironmentVariable('AWE64_ORIGINAL_CREATIVE_ONLY', '1', 'Process')
            [Environment]::SetEnvironmentVariable('AWE64_CREATIVE_BOOT_LOG', $sendBoot, 'Process')
            & python (Join-Path $root 'tests/WindowsRunVst3HostPcmSmoke.py') --exe $hostTestExe --plugin $pluginBinary --original --seconds $Seconds
            if ($LASTEXITCODE -ne 0) { $sendFailures += "native send pair $pair"; continue }
            & python (Join-Path $root 'tests/WindowsPrivateCreativeGuestSmoke.py') --send-trace $sendTrace --expected-sends ($pair -band 255) ($pair -shr 8)
            if ($LASTEXITCODE -ne 0) { $sendFailures += "native register pair $pair" }
            if ((Get-FileHash -Algorithm SHA256 $wetFixture).Hash -ne $wetHash) { throw 'Wet source changed.' }
        }
    } finally {
        [Environment]::SetEnvironmentVariable('AWE64_TEST_SEND_PAIR', $previousPair, 'Process')
        [Environment]::SetEnvironmentVariable('AWE64_TEST_EXTERNAL_SF2', $previousExternal, 'Process')
        [Environment]::SetEnvironmentVariable('AWE64_TEST_RAM_TONE_HZ', $previousRamHz, 'Process')
        [Environment]::SetEnvironmentVariable('EMU8K_TRACE', $previousTrace, 'Process')
        [Environment]::SetEnvironmentVariable('AWE64_ORIGINAL_CREATIVE_ONLY', $previousOriginal, 'Process')
        [Environment]::SetEnvironmentVariable('AWE64_CREATIVE_BOOT_LOG', $previousBootLog, 'Process')
    }
    # Direct reference: use exactly the same original GS macros without VST3.
    & python (Join-Path $root 'tests/WindowsPrivateCreativeGuestSmoke.py') --bundle $bundlePath --seconds $Seconds --reference --effect-reference
    $directEffectExit = $LASTEXITCODE
    & python (Join-Path $root 'tests/WindowsPrivateCreativeGuestSmoke.py') --bundle $bundlePath --seconds $Seconds --reference --effect-reference --corrected-sysex
    $correctedEffectExit = $LASTEXITCODE
    if ($correctedEffectExit -ne 0) { $ramFailures += "private one-byte SysEx correction gate" }
    # Genuine Creative DOS SysEx type change: observe specific physical
    # EMU8000 register writes, not merely queued host messages.
    $effectTrace = Join-Path $temporary 'original_effect_types.trace'
    $effectBoot = 'B' + [guid]::NewGuid().ToString('N').Substring(0,7).ToUpperInvariant() + '.LOG'
    $previousEffectTest = [Environment]::GetEnvironmentVariable('AWE64_TEST_EFFECT_TYPES','Process')
    try {
        [Environment]::SetEnvironmentVariable('AWE64_TEST_EFFECT_TYPES','1','Process')
        [Environment]::SetEnvironmentVariable('EMU8K_TRACE',$effectTrace,'Process')
        [Environment]::SetEnvironmentVariable('SDL_AUDIODRIVER','dummy','Process')
        [Environment]::SetEnvironmentVariable('AWE64_ORIGINAL_CREATIVE_ONLY','1','Process')
        [Environment]::SetEnvironmentVariable('AWE64_CREATIVE_BOOT_LOG',$effectBoot,'Process')
        & python (Join-Path $root 'tests/WindowsRunVst3HostPcmSmoke.py') --exe $hostTestExe --plugin $pluginBinary --original --seconds $Seconds
        if ($LASTEXITCODE -ne 0) { throw 'Original effect type native VST3 gate failed.' }
        $pointer = -1
        $plate = $false
        $feedback = $false
        foreach ($line in [System.IO.File]::ReadLines($effectTrace)) {
            if ($line -match '^(\d+) (620|622|A20|A22|E20|E22) ([0-9A-Fa-f]{4})$') {
                $frame = [long]$Matches[1]
                $port = $Matches[2]
                $word = [Convert]::ToInt32($Matches[3],16)
                if ($port -eq 'E22') { $pointer = $word -band 255 }
                if ($frame -gt 176400 -and $port -eq 'A20') {
                    if ($pointer -eq 0x43 -and $word -eq 0xB4FF) { $plate = $true }
                    if ($pointer -eq 0x69 -and $word -eq 0xE6C0) { $feedback = $true }
                }
            }
        }
        Write-Host "EFFECT_TRACE_PLATE=$plate; EFFECT_TRACE_FEEDBACK=$feedback"
        if (-not ($plate -and $feedback)) { throw 'Original DOS driver did not write expected plate/short-delay-feedback EMU8000 registers.' }
        Write-Host 'ORIGINAL_CREATIVE_EFFECT_TYPES_REGISTER_PASS; PLATE; SHORT_DELAY_FEEDBACK'
    } finally {
        [Environment]::SetEnvironmentVariable('AWE64_TEST_EFFECT_TYPES',$previousEffectTest,'Process')
        [Environment]::SetEnvironmentVariable('EMU8K_TRACE',$previousTrace,'Process')
        [Environment]::SetEnvironmentVariable('SDL_AUDIODRIVER',$previousSdl,'Process')
        [Environment]::SetEnvironmentVariable('AWE64_ORIGINAL_CREATIVE_ONLY',$previousOriginal,'Process')
        [Environment]::SetEnvironmentVariable('AWE64_CREATIVE_BOOT_LOG',$previousBootLog,'Process')
    }
    # Independent secondary original-Creative test in an isolated DLL child.
    & python (Join-Path $root 'tests/WindowsPrivateCreativeGuestSmoke.py') --bundle $bundlePath --seconds $Seconds
    if ($LASTEXITCODE -ne 0) { throw 'Private Creative ROM/SF2/DEMO DLL-level PCM gate FAILED.' }
    if ($ramFailures.Count -ne 0) { throw ("RAM gates failed: " + ($ramFailures -join "; ")) }
    if ($directEffectExit -ne 0) { throw "Direct original Creative effect gate failed." }
    if ($sendFailures.Count -ne 0) { throw ("Original send endpoint gates failed: " + ($sendFailures -join "; ")) }
    if ($cheezExit -ne 0) { throw 'Cheezoids all-program comparison failed.' }
    # Only announce final PASS after BOTH independent tests and all original
    # owner SHA-256/Git-blob and EMU8000 trace gates have actually succeeded.
    Write-Host "PRIVATE_CREATIVE_VST3_EMU8000_PCM_PASS; PORT_WRITES=$writes; ORIGINAL_ROM=VERIFIED; MIDI_NOTE_ON_OFF=VERIFIED"
} finally {
    Remove-Item -LiteralPath $temporary -Recurse -Force -ErrorAction SilentlyContinue
}
