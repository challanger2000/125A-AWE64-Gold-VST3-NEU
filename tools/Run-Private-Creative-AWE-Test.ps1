# Private owner-only test. NO original Creative binary/ROM is uploaded.
param(
    [Parameter(Mandatory=$true)][string]$Bundle,
    [Parameter(Mandatory=$true)][string]$Rom,
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
    & python (Join-Path $root 'tests/WindowsPrivateCreativeGuestSmoke.py') --bundle $bundlePath --seconds $Seconds
    if ($LASTEXITCODE -ne 0) { throw 'Private Creative ROM/SF2/DEMO PCM gate FAILED.' }
} finally {
    Remove-Item -LiteralPath $temporary -Recurse -Force -ErrorAction SilentlyContinue
}
