#requires -Version 5.1
# Decodes the base64 PNG that EditorToolset.EditorAppToolset.CaptureViewport returns.
# Large MCP tool results are spilled to a text file by the harness; pass that file here.
#
#   .\DecodeMcpCapture.ps1 -SpillPath <spill.txt> -OutFile view.png

param(
    [Parameter(Mandatory = $true)][string]$SpillPath,
    [Parameter(Mandatory = $true)][string]$OutFile
)

$text = [System.IO.File]::ReadAllText($SpillPath)
$match = [regex]::Match($text, '"data":"([A-Za-z0-9+/=]+)"')
if (-not $match.Success) {
    throw "No base64 image data found in $SpillPath"
}

$bytes = [Convert]::FromBase64String($match.Groups[1].Value)
$dir = Split-Path -Parent $OutFile
if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
[System.IO.File]::WriteAllBytes($OutFile, $bytes)
Write-Host "wrote $($bytes.Length) bytes to $OutFile"
