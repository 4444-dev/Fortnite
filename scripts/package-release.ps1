param(
    [Parameter(Mandatory = $true)]
    [string]$Version,

    [ValidateSet("Release")]
    [string]$Configuration = "Release",

    [string]$OutputDirectory = ""
)

$ErrorActionPreference = "Stop"

if ($Version.StartsWith("v", [System.StringComparison]::OrdinalIgnoreCase)) {
    $Version = $Version.Substring(1)
}

if ($Version -notmatch '^(?<major>\d+)\.(?<minor>\d+)\.(?<patch>\d+)([-.][0-9A-Za-z.-]+)?$') {
    throw "Version '$Version' is not a supported semantic version."
}

$numericVersion = "{0}.{1}.{2}.0" -f $Matches.major, $Matches.minor, $Matches.patch
$root = Split-Path -Parent $PSScriptRoot

if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $root "dist"
}

$output = [System.IO.Path]::GetFullPath($OutputDirectory)
$stage = Join-Path $output ("stage\Nexus-" + $Version)

$loaderSource = Join-Path $root "loader\bin\x64\$Configuration\Nexus.exe"
$fortniteSource = Join-Path $root "luvkrimes base\x64\$Configuration\Nexus-Fortnite.exe"

foreach ($required in @($loaderSource, $fortniteSource)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Required release binary is missing: $required"
    }
}

if (Test-Path -LiteralPath $output) {
    Remove-Item -LiteralPath $output -Recurse -Force
}

New-Item -ItemType Directory -Force -Path $stage | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $stage "projects\fortnite") | Out-Null

Copy-Item -LiteralPath $loaderSource -Destination (Join-Path $stage "Nexus.exe")
Copy-Item -LiteralPath $fortniteSource -Destination (Join-Path $stage "projects\fortnite\Nexus-Fortnite.exe")

Set-Content -LiteralPath (Join-Path $stage "VERSION.txt") -Value $Version -Encoding utf8NoBOM

$readme = @"
Nexus $Version

1. Launch Nexus.exe.
2. Select the configured product.
3. Authenticate with the license key supplied for that product.
4. Use LAUNCH after authentication.

Packaged Fortnite executable:
projects\fortnite\Nexus-Fortnite.exe

Optional development override:
NEXUS_TARGET_FORTNITE

Local settings, logs and remembered licenses:
%LOCALAPPDATA%\Nexus

Visual Studio and the source repository are not required.
"@

Set-Content -LiteralPath (Join-Path $stage "README.txt") -Value $readme -Encoding utf8NoBOM

$portablePath = Join-Path $output ("Nexus-Portable-" + $Version + ".zip")
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $portablePath -CompressionLevel Optimal

$makensisCommand = Get-Command "makensis.exe" -ErrorAction SilentlyContinue
$makensisPath = if ($makensisCommand) { $makensisCommand.Source } else { "" }

if ([string]::IsNullOrWhiteSpace($makensisPath)) {
    $candidates = @(
        "$env:ProgramFiles\NSIS\makensis.exe",
        "${env:ProgramFiles(x86)}\NSIS\makensis.exe"
    )

    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            $makensisPath = $candidate
            break
        }
    }
}

if ([string]::IsNullOrWhiteSpace($makensisPath)) {
    throw "makensis.exe was not found. Install NSIS before packaging."
}

$installerScript = Join-Path $root "installer\nexus.nsi"

& $makensisPath `
    "/DVERSION=$Version" `
    "/DNUMERICVERSION=$numericVersion" `
    "/DSOURCEDIR=$stage" `
    "/DOUTDIR=$output" `
    $installerScript

if ($LASTEXITCODE -ne 0) {
    throw "NSIS failed with exit code $LASTEXITCODE."
}

$installerPath = Join-Path $output ("Nexus-Setup-" + $Version + ".exe")
if (-not (Test-Path -LiteralPath $installerPath -PathType Leaf)) {
    throw "Installer was not created: $installerPath"
}

$checksumPath = Join-Path $output "SHA256SUMS.txt"
$assets = @($installerPath, $portablePath)

$checksumLines = foreach ($asset in $assets) {
    $hash = Get-FileHash -LiteralPath $asset -Algorithm SHA256
    "{0}  {1}" -f $hash.Hash.ToLowerInvariant(), (Split-Path -Leaf $asset)
}

Set-Content -LiteralPath $checksumPath -Value $checksumLines -Encoding ascii

Write-Host "Release package created:"
Write-Host "  $installerPath"
Write-Host "  $portablePath"
Write-Host "  $checksumPath"
