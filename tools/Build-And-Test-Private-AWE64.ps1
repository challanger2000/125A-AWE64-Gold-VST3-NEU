# Owner-only Windows build + REAL ORIGINAL Creative DOS AWE64 sound acceptance.
# No original ROM, Creative EXE, SF2, PCM or traces are uploaded or committed.
# This is a development test, not a release or a Studio Pro test.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Rom,
    [int]$Seconds = 60,
    [string]$Vst3Sdk = ''
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($env:OS -ne 'Windows_NT' -or -not [Environment]::Is64BitOperatingSystem) {
    throw 'Windows x64 is required.'
}
if ($Seconds -lt 15 -or $Seconds -gt 120) { throw 'Seconds must be 15..120.' }

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$romPath = (Resolve-Path $Rom).Path
$expectedRom = '4e143b94f758734f594ded78f4e5115635975c16fa37fabeae0baa4938ca710e'
if ((Get-Item -LiteralPath $romPath).Length -ne 1048576 -or
    (Get-FileHash -Algorithm SHA256 -LiteralPath $romPath).Hash.ToLowerInvariant() -ne $expectedRom) {
    throw 'Owner AWE32 Wave-ROM is missing, damaged or not the exact 1-MiB original.'
}

# Verify original, pinned DOSBox-X core without resetting owner edits.
$sub = Join-Path $root 'third_party/dosbox-x-AWE32'
if (-not (Test-Path -PathType Leaf (Join-Path $sub 'vs/dosbox-x.sln'))) {
    & git -C $root submodule update --init --recursive
    if ($LASTEXITCODE -ne 0) { throw 'Pinned DOSBox-X submodule is unavailable.' }
}
$pinned = '81d0fde73917b57a1716e610dec2605c36def555'
$actual = (& git -C $sub rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $actual -ne $pinned) {
    throw "Unexpected DOSBox-X source SHA: $actual"
}
# Never reuse a patched source tree, even if its upstream HEAD is correct.
# The previous patch marker did not identify the actual current patch.
$privateRoot = Join-Path $root 'PRIVATE'
$prepared = @(& python (Join-Path $root 'tools/prepare_private_source.py') $sub $privateRoot (Join-Path $root 'integration/patch_dosbox_runtime.py'))
if ($LASTEXITCODE -ne 0 -or $prepared.Count -ne 1) { throw 'Fresh private DOSBox source preparation failed.' }
$sub = $prepared[0]
$runRoot = Split-Path $sub -Parent
$patchManifest = Join-Path $runRoot 'source-provenance.json'
if (-not (Test-Path -PathType Leaf $patchManifest)) { throw 'Private source provenance is absent.' }

$vswhere = Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Microsoft Visual Studio/Installer/vswhere.exe'
if (-not (Test-Path -PathType Leaf $vswhere)) { throw 'Visual Studio 2022 vswhere.exe missing.' }
$vsResult = @(& $vswhere -version '[17.0,18.0)' -products '*' -requires Microsoft.Component.MSBuild -property installationPath)
if ($LASTEXITCODE -ne 0 -or $vsResult.Count -lt 1) { throw 'MSBuild from Visual Studio 2022 is required.' }
$msbuild = Join-Path $vsResult[0] 'MSBuild/Current/Bin/MSBuild.exe'
if (-not (Test-Path -PathType Leaf $msbuild)) { throw 'MSBuild.exe not found.' }
& $msbuild (Join-Path $sub 'vs/dosbox-x.sln') '-t:dosbox-x:Rebuild' '-p:Configuration=Release SDL2' '-p:Platform=x64' '-p:PostBuildEventUseInBuild=false' '-m:4'
if ($LASTEXITCODE -ne 0) { throw 'Original DOSBox-X AWE32 x64 DLL build failed.' }
$runtime = Join-Path $sub 'bin/x64/Release SDL2/AWE64DOSBoxRuntime.dll'
if (-not (Test-Path -PathType Leaf $runtime)) { throw 'Original Windows DOSBox-X runtime DLL missing.' }

# Steinberg SDK is used only at compile time, never bundled as a new host.
if ($Vst3Sdk) {
    $sdk = (Resolve-Path $Vst3Sdk).Path
} else {
    $sdk = Join-Path $privateRoot 'toolchains/vst3sdk'
    if (-not (Test-Path -PathType Leaf (Join-Path $sdk 'public.sdk/source/vst/vstsinglecomponenteffect.cpp'))) {
        New-Item -ItemType Directory -Path (Split-Path $sdk -Parent) -Force | Out-Null
        & git clone --depth 1 --branch v3.7.12_build_20 --recurse-submodules --shallow-submodules https://github.com/steinbergmedia/vst3sdk.git $sdk
        if ($LASTEXITCODE -ne 0) { throw 'Pinned Steinberg VST3 SDK checkout failed.' }
    }
}
if (-not (Test-Path -PathType Leaf (Join-Path $sdk 'public.sdk/source/vst/vstsinglecomponenteffect.cpp'))) {
    throw 'Complete Steinberg VST3 SDK 3.7.12 is missing.'
}
$sdkTag = @(& git -C $sdk describe --tags --exact-match)
if ($LASTEXITCODE -ne 0 -or $sdkTag.Count -ne 1 -or $sdkTag[0] -ne 'v3.7.12_build_20') {
    throw 'Refusing unpinned Steinberg SDK.'
}

$build = Join-Path $runRoot 'build-vst3'
& cmake -S $root -B $build -G 'Visual Studio 17 2022' -A x64 '-DAWE64_BUILD_VST3_DEV=ON' "-DAWE64_VST3SDK=$sdk" '-DBUILD_TESTING=OFF'
if ($LASTEXITCODE -ne 0) { throw 'Original P165 VST3 CMake configure failed.' }
& cmake --build $build --config Release --target AWE64GoldDOSBoxDev awe64_vst3_host_pcm_smoke --parallel 2
if ($LASTEXITCODE -ne 0) { throw 'Actual Windows VST3 and native test host build failed.' }
$bundle = Join-Path $build 'VST3/Release/125A AWE64 Gold.vst3'
$native = Join-Path $bundle 'Contents/x86_64-win'
if (-not (Test-Path -PathType Leaf (Join-Path $native '125A AWE64 Gold.vst3'))) {
    throw 'Actual Windows VST3 binary is missing.'
}
$stagedRuntime = Join-Path $native 'Engine.dll'
Copy-Item -LiteralPath $runtime -Destination $stagedRuntime
$runtimeHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $runtime).Hash
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $stagedRuntime).Hash -ne $runtimeHash) {
    throw 'Staged runtime SHA-256 differs from freshly built DLL.'
}
$plugin = Join-Path $native '125A AWE64 Gold.vst3'
$binaryHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $plugin).Hash
$productHead = (& git -C $root rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Cannot identify product source HEAD.' }
$manifest = @{
    product_head = $productHead
    product_worktree_status = @(& git -C $root status --porcelain --untracked-files=all)
    source = (Get-Content -LiteralPath $patchManifest -Raw | ConvertFrom-Json)
    runtime_sha256 = $runtimeHash
    vst3_sha256 = $binaryHash
    sdk_tag = $sdkTag[0]
}
if ($LASTEXITCODE -ne 0) { throw 'Cannot identify product worktree status.' }
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $runRoot 'build-provenance.json') -Encoding UTF8
Write-Host "PRIVATE_BUILD_BUNDLE=$bundle"

# This PRIVATE script stages the original hash-verified Creative developer EXE,
# original SF2 and owner ROM locally, then checks real VST3 NoteOn/NoteOff,
# original EMU8000 hardware writes and audible VST3 PCM. No DAW yet.
& powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File (Join-Path $root 'tools/Run-Private-Creative-AWE-Test.ps1') -Bundle $bundle -Rom $romPath -Seconds $Seconds -HostExe (Join-Path $build 'Release/awe64_vst3_host_pcm_smoke.exe')
if ($LASTEXITCODE -ne 0) { throw 'PRIVATE ORIGINAL CREATIVE SOUND NOT PROVEN; no release.' }
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $stagedRuntime).Hash -ne $runtimeHash -or
    (Get-FileHash -Algorithm SHA256 -LiteralPath $plugin).Hash -ne $binaryHash) {
    throw 'Tested plugin binaries changed during acceptance.'
}
Write-Host 'PRIVATE ORIGINAL CREATIVE AWE64 VST3 SOUND ACCEPTANCE PASS'
Write-Host 'Still 0.0.0-dev: final Studio Pro/GUI/state acceptance not yet performed.'
