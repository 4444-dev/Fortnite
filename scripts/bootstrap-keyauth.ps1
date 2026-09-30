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

# Upstream currently contains one unguarded LockMemAccess() call even though
# Security.hpp is optional. Guard that call so the SDK still compiles when the
# optional anti-analysis module is intentionally excluded.
$authCpp = Join-Path $target "auth.cpp"
$authText = Get-Content $authCpp -Raw
$lockMemPattern = '(?ms)^(?<indent>[ \\t]*)if\\s*\\(\\s*!LockMemAccess\\(\\)\\s*\\)\\s*\\r?\\n\\k<indent>\\{\\s*\\r?\\n\\k<indent>[ \\t]+error\\(XorStr\\("LockMemAccess\\(\\) failed, don''t tamper with the program\\."\\)\\);\\s*\\r?\\n\\k<indent>\\}'
$lockMemMatches = [regex]::Matches($authText, $lockMemPattern)
if ($lockMemMatches.Count -ne 1) {
    throw "Expected exactly one unguarded LockMemAccess block, found $($lockMemMatches.Count)."
}
$authText = [regex]::Replace(
    $authText,
    $lockMemPattern,
    { param($match) "#if KEYAUTH_HAVE_SECURITY`r`n$($match.Value)`r`n#endif" },
    1
)
Set-Content -Path $authCpp -Value $authText -NoNewline

# Upstream Tfa::handleInput() is declared to return Tfa& but currently falls
# through without a return. Add the missing return so MSVC can compile it.
$tfaTail = @'
		instance.disable2fa(code);
	}

}

void KeyAuth::api::web_login()
'@
$tfaFixed = @'
		instance.disable2fa(code);
	}

    return *this;
}

void KeyAuth::api::web_login()
'@
if ($authText.Contains($tfaTail)) {
    $authText = $authText.Replace($tfaTail, $tfaFixed)
    Set-Content -Path $authCpp -Value $authText -NoNewline
}

$actual = Get-CurrentCommit
if ($actual -ne $pinnedCommit) {
    throw "KeyAuth bootstrap verification failed. Expected $pinnedCommit, got $actual."
}

Write-Host "KeyAuth 1.3 ready at pinned commit $actual"
