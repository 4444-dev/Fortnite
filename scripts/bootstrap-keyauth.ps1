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

# The upstream library conditionally enables these modules when the files exist.
# Authentication remains enabled; these optional anti-analysis/emulator modules
# are intentionally excluded from this project.
Remove-Item (Join-Path $target "Security.hpp") -Force -ErrorAction SilentlyContinue
Remove-Item (Join-Path $target "killEmulator.hpp") -Force -ErrorAction SilentlyContinue

$actual = Get-CurrentCommit
if ($actual -ne $pinnedCommit) {
    throw "KeyAuth bootstrap verification failed. Expected $pinnedCommit, got $actual."
}

Write-Host "KeyAuth 1.3 ready at pinned commit $actual"
