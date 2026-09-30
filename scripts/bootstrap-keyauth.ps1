$ErrorActionPreference = "Stop"

$repoUrl = "https://github.com/KeyAuth/keyauth-cpp-library-1.3API.git"
$pinnedCommit = "486c83e6259f508ba0396f3156e50792a34a4576"
$root = Split-Path -Parent $PSScriptRoot
$target = Join-Path $root "thirdparty\keyauth"

function Get-CurrentCommit {
    if (-not (Test-Path (Join-Path $target ".git"))) {
        return ""
    }

    Push-Location $target
    try {
        return (git rev-parse HEAD).Trim()
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
        git remote add origin $repoUrl
        git fetch --depth 1 origin $pinnedCommit
        git checkout --detach FETCH_HEAD
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
$lines = [System.Collections.Generic.List[string]]::new()
foreach ($line in (Get-Content $authCpp)) {
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

Set-Content -Path $authCpp -Value $lines

$actual = Get-CurrentCommit
if ($actual -ne $pinnedCommit) {
    throw "KeyAuth bootstrap verification failed. Expected $pinnedCommit, got $actual."
}

Write-Host "KeyAuth 1.3 ready at pinned commit $actual"
