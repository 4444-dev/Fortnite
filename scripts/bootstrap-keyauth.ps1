$ErrorActionPreference = "Stop"

$repoUrl = "https://github.com/KeyAuth/keyauth-cpp-library-1.3API.git"
$pinnedCommit = "486c83e6259f508ba0396f3156e50792a34a4576"
$root = Split-Path -Parent $PSScriptRoot
$target = Join-Path $root "thirdparty\keyauth"

function Assert-GitSuccess([string]$operation) {
    if ($LASTEXITCODE -ne 0) {
        throw "Git operation failed: $operation (exit code $LASTEXITCODE)."
    }
}

function Get-CurrentCommit {
    if (-not (Test-Path (Join-Path $target ".git"))) {
        return ""
    }

    Push-Location $target
    try {
        $commit = git rev-parse HEAD
        Assert-GitSuccess "rev-parse HEAD"
        return ($commit | Out-String).Trim()
    }
    finally {
        Pop-Location
    }
}

$current = Get-CurrentCommit
if ($current -ne $pinnedCommit) {
    if (Test-Path $target) {
        Remove-Item $target -Recurse -Force
    }

    New-Item -ItemType Directory -Force -Path $target | Out-Null

    Push-Location $target
    try {
        git init | Out-Null
        Assert-GitSuccess "init"

        git remote add origin $repoUrl
        Assert-GitSuccess "remote add origin"

        git fetch --depth 1 origin $pinnedCommit
        Assert-GitSuccess "fetch pinned KeyAuth commit"

        git checkout --detach FETCH_HEAD | Out-Null
        Assert-GitSuccess "checkout pinned KeyAuth commit"
    }
    finally {
        Pop-Location
    }
}
else {
    # The dependency directory is generated and ignored. Reset it before
    # applying our compatibility patch so repeated local builds are
    # deterministic and cannot stack duplicate preprocessor guards.
    Push-Location $target
    try {
        git reset --hard $pinnedCommit | Out-Null
        Assert-GitSuccess "reset pinned KeyAuth commit"
    }
    finally {
        Pop-Location
    }
}

# Authentication remains enabled. These optional upstream anti-analysis/emulator
# modules are intentionally excluded from this project.
Remove-Item (Join-Path $target "Security.hpp") -Force -ErrorAction SilentlyContinue
Remove-Item (Join-Path $target "killEmulator.hpp") -Force -ErrorAction SilentlyContinue

$authCpp = Join-Path $target "auth.cpp"
if (-not (Test-Path -LiteralPath $authCpp -PathType Leaf)) {
    throw "KeyAuth bootstrap is incomplete: auth.cpp was not found."
}

$lines = [System.Collections.Generic.List[string]]::new()
foreach ($line in (Get-Content -LiteralPath $authCpp)) {
    $lines.Add($line)
}

# Upstream has one unguarded LockMemAccess() block even though Security.hpp is
# optional. Guard only that block when Security.hpp has intentionally been
# removed.
$lockIndex = -1
for ($i = 0; $i -lt $lines.Count; $i++) {
    if ($lines[$i] -match '^\s*if\s*\(\s*!LockMemAccess\(\)\s*\)') {
        $lockIndex = $i
        break
    }
}

if ($lockIndex -lt 0) {
    throw "Could not locate the unguarded LockMemAccess() block."
}

$lockEnd = -1
for ($i = $lockIndex + 1; $i -lt $lines.Count; $i++) {
    if ($lines[$i].Trim() -eq "}") {
        $lockEnd = $i
        break
    }
}

if ($lockEnd -lt 0) {
    throw "Could not locate the end of the LockMemAccess() block."
}

$lines.Insert($lockEnd + 1, "#endif")
$lines.Insert($lockIndex, "#if KEYAUTH_HAVE_SECURITY")

# Upstream Tfa::handleInput() is declared to return Tfa& but falls through
# without returning. Add the missing return at the function's final brace.
$tfaStart = -1
for ($i = 0; $i -lt $lines.Count; $i++) {
    if ($lines[$i] -match '^KeyAuth::api::Tfa&\s+KeyAuth::api::Tfa::handleInput') {
        $tfaStart = $i
        break
    }
}

if ($tfaStart -lt 0) {
    throw "Could not locate KeyAuth Tfa::handleInput()."
}

$depth = 0
$started = $false
$tfaEnd = -1

for ($i = $tfaStart; $i -lt $lines.Count; $i++) {
    $openCount = ([regex]::Matches($lines[$i], '\{')).Count
    $closeCount = ([regex]::Matches($lines[$i], '\}')).Count

    if ($openCount -gt 0) {
        $started = $true
    }

    $depth += $openCount
    $depth -= $closeCount

    if ($started -and $depth -eq 0) {
        $tfaEnd = $i
        break
    }
}

if ($tfaEnd -lt 0) {
    throw "Could not locate the end of KeyAuth Tfa::handleInput()."
}

$alreadyReturns = $false
for ($i = $tfaStart; $i -le $tfaEnd; $i++) {
    if ($lines[$i] -match 'return\s+\*this\s*;') {
        $alreadyReturns = $true
        break
    }
}

if (-not $alreadyReturns) {
    $lines.Insert($tfaEnd, "    return *this;")
}

function Add-FallbackReturn(
    [System.Collections.Generic.List[string]]$source,
    [string]$signatureRegex,
    [string]$returnLine,
    [string]$description
) {
    $start = -1
    for ($i = 0; $i -lt $source.Count; $i++) {
        if ($source[$i] -match $signatureRegex) {
            $start = $i
            break
        }
    }

    if ($start -lt 0) {
        throw "Could not locate KeyAuth function: $description."
    }

    $depth = 0
    $started = $false
    $end = -1

    for ($i = $start; $i -lt $source.Count; $i++) {
        $openCount = ([regex]::Matches($source[$i], '\{')).Count
        $closeCount = ([regex]::Matches($source[$i], '\}')).Count

        if ($openCount -gt 0) {
            $started = $true
        }

        $depth += $openCount
        $depth -= $closeCount

        if ($started -and $depth -eq 0) {
            $end = $i
            break
        }
    }

    if ($end -lt 0) {
        throw "Could not locate the end of KeyAuth function: $description."
    }

    if ($end -gt $start -and $source[$end - 1].Trim() -ne $returnLine.Trim()) {
        $source.Insert($end, $returnLine)
    }
}

# Several upstream functions terminate through KA_EXIT on error paths. MSVC
# cannot prove that those paths never return, so add explicit defensive
# fallbacks to keep control-flow contracts well-formed.
Add-FallbackReturn $lines '^std::string KeyAuth::api::getvar\(' '    return {};' 'getvar'
Add-FallbackReturn $lines '^bool KeyAuth::api::checkblack\(' '    return false;' 'checkblack'
Add-FallbackReturn $lines '^std::string KeyAuth::api::var\(' '    return {};' 'var'
Add-FallbackReturn $lines '^std::string KeyAuth::api::webhook\(' '    return {};' 'webhook'
Add-FallbackReturn $lines '^std::string KeyAuth::api::fetchonline\(' '    return {};' 'fetchonline'

$text = $lines -join [Environment]::NewLine

$oldFilenameConversion = @'
    std::wstring filenameOnlyString(filename_only);

    std::string filenameOnly(filenameOnlyString.begin(), filenameOnlyString.end());
'@

$newFilenameConversion = @'
    std::wstring filenameOnlyString(filename_only);

    std::string filenameOnly;
    const int utf8Size = WideCharToMultiByte(
        CP_UTF8,
        0,
        filenameOnlyString.c_str(),
        -1,
        nullptr,
        0,
        nullptr,
        nullptr
    );
    if (utf8Size > 1) {
        filenameOnly.resize(static_cast<std::size_t>(utf8Size));
        const int converted = WideCharToMultiByte(
            CP_UTF8,
            0,
            filenameOnlyString.c_str(),
            -1,
            filenameOnly.data(),
            utf8Size,
            nullptr,
            nullptr
        );
        if (converted > 0) {
            filenameOnly.resize(static_cast<std::size_t>(converted - 1));
        }
        else {
            filenameOnly.clear();
        }
    }
'@

if (-not $text.Contains($oldFilenameConversion)) {
    throw "Could not locate KeyAuth filename conversion compatibility block."
}
$text = $text.Replace($oldFilenameConversion, $newFilenameConversion)

Set-Content -LiteralPath $authCpp -Value $text

$pingoutCpp = Join-Path $target "QRCode\pingout.cpp"
if (-not (Test-Path -LiteralPath $pingoutCpp -PathType Leaf)) {
    throw "KeyAuth bootstrap is incomplete: QRCode/pingout.cpp was not found."
}

$pingout = Get-Content -LiteralPath $pingoutCpp -Raw
$invalidPragma = "#pragma warning(disable:C4146)"
if (-not $pingout.Contains($invalidPragma)) {
    throw "Could not locate the malformed KeyAuth QRCode warning pragma."
}
$pingout = $pingout.Replace($invalidPragma, "")
Set-Content -LiteralPath $pingoutCpp -Value $pingout

$actual = Get-CurrentCommit
if ($actual -ne $pinnedCommit) {
    throw "KeyAuth bootstrap verification failed. Expected $pinnedCommit, got $actual."
}

Write-Host "KeyAuth 1.3 ready at pinned commit $actual"
