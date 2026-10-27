param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^\d+(\.\d+){3}$')]
    [string]$Version,
    [string]$ResumeDirectory,
    [ValidateSet('ZbouhaKahlin/Azec')]
    [string]$Repository = 'ZbouhaKahlin/Azec'
)

$ErrorActionPreference = 'Stop'
# Developer shells can inherit module paths for another PowerShell version.
# Load the built-in modules from this executable's own installation explicitly.
foreach ($moduleName in @('Microsoft.PowerShell.Management', 'Microsoft.PowerShell.Utility')) {
    Import-Module (Join-Path $PSHOME "Modules\$moduleName\$moduleName.psd1") -ErrorAction Stop
}
$repo = $Repository
$tag = "v$Version"
$localExe = Join-Path $PSScriptRoot 'dist\Azec.exe'
$versionHeader = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src\version.hpp') -Raw
if ($versionHeader -notmatch ('#define AZEC_VERSION "' + [regex]::Escape($Version) + '"')) {
    throw 'Published version does not match src/version.hpp.'
}
$localHash = (Get-FileHash -LiteralPath $localExe -Algorithm SHA256).Hash.ToLowerInvariant()
$headers = @{ 'User-Agent' = 'Azec-release-verification'; 'Accept' = 'application/vnd.github+json' }
$release = $null
for ($attempt = 0; $attempt -lt 5; $attempt++) {
    $release = Invoke-RestMethod -Uri "https://api.github.com/repos/$repo/releases/tags/$tag" -Headers $headers -TimeoutSec 30
    $assets = @($release.assets)
    if ($assets.Count -eq 2 -and @($assets | Where-Object { $_.digest -notmatch '^sha256:[0-9a-fA-F]{64}$' }).Count -eq 0) { break }
    if ($attempt -lt 4) { Start-Sleep -Seconds 2 }
}
if ($release.tag_name -cne $tag -or $release.draft -or $release.prerelease) {
    throw 'The uploaded release is not the expected stable tag.'
}
if ($assets.Count -ne 2 -or (@($assets.name | Sort-Object) -join ',') -cne 'Azec.exe,SHA256SUMS.txt') {
    throw 'The public release must contain Azec.exe and SHA256SUMS.txt only.'
}
$asset = @($assets | Where-Object { $_.name -ceq 'Azec.exe' })[0]
$checksumAsset = @($assets | Where-Object { $_.name -ceq 'SHA256SUMS.txt' })[0]
$checksumFile = Join-Path $PSScriptRoot 'dist/SHA256SUMS.txt'
$checksumHash = (Get-FileHash -LiteralPath $checksumFile -Algorithm SHA256).Hash.ToLowerInvariant()
if ($checksumAsset.digest -cne ('sha256:' + $checksumHash) -or (Get-Content -LiteralPath $checksumFile -Raw).Trim() -cne ($localHash + '  Azec.exe')) { throw 'Published checksum file does not match the installer record.' }
$expectedCommit = & git -C $PSScriptRoot rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Cannot resolve local source commit.' }
$sourceCommit = Invoke-RestMethod -Uri "https://api.github.com/repos/ZbouhaKahlin/Azec/commits/$tag" -Headers $headers -TimeoutSec 30
if ($sourceCommit.sha -cne $expectedCommit.Trim()) { throw 'The release tag does not identify the built source commit.' }
if ($asset.digest -notmatch '^sha256:([0-9a-fA-F]{64})$') { throw 'GitHub asset SHA-256 digest is missing.' }
$publishedHash = $Matches[1].ToLowerInvariant()
if ($publishedHash -cne $localHash) { throw 'GitHub asset digest differs from dist/Azec.exe.' }

$verificationRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '.build\verification'))
if ($ResumeDirectory) {
    $fixture = [IO.Path]::GetFullPath($ResumeDirectory)
    if (-not $fixture.StartsWith($verificationRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Release verification downloads must stay under .build/verification.'
    }
} else {
    $fixture = Join-Path $verificationRoot ('published-' + [guid]::NewGuid().ToString('N'))
}
New-Item -ItemType Directory -Path $fixture -Force | Out-Null
Write-Host "Release verification downloads: $fixture"
$taggedExe = Join-Path $fixture 'tagged.exe'
$stableExe = Join-Path $fixture 'latest.exe'
# These requests deliberately carry no GitHub token: friends use anonymous downloads.
function Download-AnonymousRelease([string]$Uri, [string]$Destination) {
    if (Test-Path -LiteralPath $Destination) {
        $length = (Get-Item -LiteralPath $Destination).Length
        if ($length -eq $asset.size -and (Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash.ToLowerInvariant() -ceq $publishedHash) { return }
        if ($length -ge $asset.size) { throw 'Existing verification download is invalid; retry in a fresh directory.' }
    }
    # Bound connection and transfer time separately; ignore local curl credentials/config.
    & curl.exe --disable --fail --location --silent --show-error --proto '=https' --proto-redir '=https' --connect-timeout 20 --max-time 600 --continue-at - --output $Destination -- $Uri
    if ($LASTEXITCODE -ne 0) { throw "Anonymous release download failed (curl exit $LASTEXITCODE)." }
}
Download-AnonymousRelease "https://github.com/$repo/releases/download/$tag/Azec.exe" $taggedExe
if ((Get-FileHash -LiteralPath $taggedExe -Algorithm SHA256).Hash.ToLowerInvariant() -cne $publishedHash) {
    throw 'Downloaded tagged EXE differs from the asset digest.'
}
$latest = Invoke-RestMethod -Uri "https://api.github.com/repos/$repo/releases/latest" -Headers $headers -TimeoutSec 30
if ($latest.tag_name -cne $tag) { throw 'The stable latest release points to another version.' }
Download-AnonymousRelease "https://github.com/$repo/releases/latest/download/Azec.exe" $stableExe
if ((Get-FileHash -LiteralPath $stableExe -Algorithm SHA256).Hash.ToLowerInvariant() -cne $publishedHash) {
    throw 'The anonymous stable download differs from the published EXE.'
}
Write-Host "Verified $repo $tag : source tag, local EXE, GitHub digest, tagged download, and anonymous latest download match."
