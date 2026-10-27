param(
    [switch]$Test,
    [ValidatePattern('^\d+(\.\d+){3}$')]
    [string]$ResumeVersion
)

$ErrorActionPreference = 'Stop'
foreach ($moduleName in @('Microsoft.PowerShell.Management', 'Microsoft.PowerShell.Utility')) {
    Import-Module (Join-Path $PSHOME "Modules\$moduleName\$moduleName.psd1") -ErrorAction Stop
}
Set-Location $PSScriptRoot
$repo = 'ZbouhaKahlin/Azec'

function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed (exit $LASTEXITCODE). Publication is incomplete." }
}

$branch = & git branch --show-current
if ($LASTEXITCODE -ne 0 -or $branch -cne 'main') { throw 'Release publication requires the main branch.' }
$remote = & git remote get-url origin
if ($LASTEXITCODE -ne 0 -or $remote -notmatch '^https://github\.com/ZbouhaKahlin/Azec(?:\.git)?/?$|^git@github\.com:ZbouhaKahlin/Azec(?:\.git)?$') {
    throw 'origin must point to ZbouhaKahlin/Azec.'
}
$status = & git status --porcelain
if ($LASTEXITCODE -ne 0 -or $status) { throw 'Commit the intended source changes first; the release checkout must be clean.' }
Invoke-Checked 'gh' @('auth', 'status')
Invoke-Checked 'gh' @('repo', 'view', 'ZbouhaKahlin/Azec', '--json', 'nameWithOwner')
Invoke-Checked 'git' @('fetch', 'origin', 'main')
Invoke-Checked 'git' @('merge-base', '--is-ancestor', 'origin/main', 'HEAD')

if ($ResumeVersion) {
    $version = $ResumeVersion
} else {
    $buildArguments = @()
    if ($Test) { $buildArguments += 'test' }
    Invoke-Checked (Join-Path $PSScriptRoot 'share.cmd') $buildArguments
    $header = Get-Content -LiteralPath 'src/version.hpp' -Raw
    if ($header -notmatch '#define AZEC_VERSION "(\d+(?:\.\d+){3})"') { throw 'The build did not produce a numeric release version.' }
    $version = $Matches[1]
    $changes = @(& git diff --name-only)
    if ($LASTEXITCODE -ne 0 -or $changes.Count -ne 1 -or $changes[0] -cne 'src/version.hpp') {
        throw 'Unexpected source changes during packaging; nothing was staged or pushed.'
    }
    Invoke-Checked 'git' @('add', '--', 'src/version.hpp')
    Invoke-Checked 'git' @('-c', 'user.name=Azec', '-c', 'user.email=293977364+ZbouhaKahlin@users.noreply.github.com', 'commit', '-m', "Release Azec $version")
}

$header = Get-Content -LiteralPath 'src/version.hpp' -Raw
if ($header -notmatch ('#define AZEC_VERSION "' + [regex]::Escape($version) + '"')) { throw 'The version does not match the committed source.' }
$exe = Join-Path $PSScriptRoot 'dist/Azec.exe'
if (-not (Test-Path -LiteralPath $exe)) { throw 'The matching installer is missing. Do not rebuild an already published version.' }
$sha = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant()
$buildVersion = (Get-Content -LiteralPath '.build/share-build/version.txt' -Raw).Trim()
if ($buildVersion -cne $version) { throw 'The installer build record does not match the requested version.' }
$checksum = Join-Path $PSScriptRoot 'dist/SHA256SUMS.txt'
if ((Get-Content -LiteralPath $checksum -Raw).Trim() -cne "$sha  Azec.exe") { throw 'Installer and recorded checksum do not match.' }
$commit = & git rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Cannot resolve the source commit.' }
Invoke-Checked 'git' @('push', 'origin', 'main')
$tag = "v$version"
$notes = Join-Path $PSScriptRoot '.build/release-notes.md'
@"
Install **Azec.exe** on Windows x64. Profiles remain in the existing Azec data folder across updates.

Source: https://github.com/$repo/commit/$commit. The source repository's matching tag identifies this build.

This build checks and downloads future updates from this repository.

SHA-256 (Azec.exe): $sha
"@ | Set-Content -LiteralPath $notes -Encoding UTF8
& gh release view $tag --repo $repo --json tagName *> $null
if ($LASTEXITCODE -ne 0) {
    Invoke-Checked 'gh' @('release', 'create', $tag, $exe, $checksum, '--repo', $repo, '--target', $commit, '--title', "Azec $version", '--notes-file', $notes, '--latest')
} else {
    Write-Host "Release $tag already exists in $repo; verifying without replacing assets."
}
Invoke-Checked 'powershell.exe' @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $PSScriptRoot 'verify-release.ps1'), '-Version', $version, '-Repository', $repo)
Write-Host "Published $tag from $commit"
Write-Host "https://github.com/$repo/releases/latest/download/Azec.exe"
