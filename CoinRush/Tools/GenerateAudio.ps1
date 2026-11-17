#requires -Version 5.1
# Generates tiny SFX as 16-bit PCM mono WAV files (no external assets, no licenses).
# Output: Tools\AudioSrc\SFX_Coin.wav, SFX_Land.wav
# Usage:  powershell -ExecutionPolicy Bypass -File Tools\GenerateAudio.ps1
# NOTE: keep ASCII-only (PowerShell 5.1 reads BOM-less files as ANSI).

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$OutDir = Join-Path $Root 'Tools\AudioSrc'
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$SampleRate = 44100

function Write-Wav([string]$Path, [double[]]$Samples, [int]$Rate) {
    $dataBytes = $Samples.Length * 2
    $stream = New-Object System.IO.MemoryStream
    $writer = New-Object System.IO.BinaryWriter($stream)

    $writer.Write([System.Text.Encoding]::ASCII.GetBytes('RIFF'))
    $writer.Write([int32](36 + $dataBytes))
    $writer.Write([System.Text.Encoding]::ASCII.GetBytes('WAVE'))
    $writer.Write([System.Text.Encoding]::ASCII.GetBytes('fmt '))
    $writer.Write([int32]16)          # PCM chunk size
    $writer.Write([int16]1)            # PCM
    $writer.Write([int16]1)            # mono
    $writer.Write([int32]$Rate)
    $writer.Write([int32]($Rate * 2))  # byte rate
    $writer.Write([int16]2)            # block align
    $writer.Write([int16]16)           # bits per sample
    $writer.Write([System.Text.Encoding]::ASCII.GetBytes('data'))
    $writer.Write([int32]$dataBytes)

    foreach ($s in $Samples) {
        $v = [Math]::Max(-1.0, [Math]::Min(1.0, $s))
        $writer.Write([int16]([Math]::Round($v * 32767)))
    }

    $writer.Flush()
    [System.IO.File]::WriteAllBytes($Path, $stream.ToArray())
    $writer.Dispose()
    $stream.Dispose()
    Write-Host ("wrote {0} ({1:N0} bytes)" -f $Path, (Get-Item $Path).Length)
}

# --- Coin "ding": two short bright partials, fast exponential decay -----------
$coinSeconds = 0.24
$coinCount = [int]($SampleRate * $coinSeconds)
$coin = New-Object double[] $coinCount
for ($i = 0; $i -lt $coinCount; $i++) {
    $t = $i / $SampleRate
    $env = [Math]::Exp(-$t * 16.0)
    $attack = [Math]::Min(1.0, $t / 0.004)
    $tone = 0.55 * [Math]::Sin(2 * [Math]::PI * 1568.0 * $t)
    $tone += 0.30 * [Math]::Sin(2 * [Math]::PI * 2093.0 * $t) * [Math]::Exp(-$t * 22.0)
    $tone += 0.15 * [Math]::Sin(2 * [Math]::PI * 3136.0 * $t) * [Math]::Exp(-$t * 30.0)
    $coin[$i] = $tone * $env * $attack * 0.9
}
Write-Wav (Join-Path $OutDir 'SFX_Coin.wav') $coin $SampleRate

# --- Land "thud": low sine + short noise burst -------------------------------
$landSeconds = 0.16
$landCount = [int]($SampleRate * $landSeconds)
$land = New-Object double[] $landCount
$rng = New-Object System.Random(12345)
for ($i = 0; $i -lt $landCount; $i++) {
    $t = $i / $SampleRate
    $thump = 0.85 * [Math]::Sin(2 * [Math]::PI * 72.0 * $t) * [Math]::Exp(-$t * 26.0)
    $noise = ($rng.NextDouble() * 2.0 - 1.0) * 0.35 * [Math]::Exp(-$t * 45.0)
    $land[$i] = ($thump + $noise) * 0.9
}
Write-Wav (Join-Path $OutDir 'SFX_Land.wav') $land $SampleRate
