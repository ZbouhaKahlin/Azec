param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^\d+(\.\d+){3}$')]
    [string]$Version
)

$ErrorActionPreference = 'Stop'
# Developer shells can inherit module paths for another PowerShell version.
# Load the built-in modules from this executable's own installation explicitly.
foreach ($moduleName in @('Microsoft.PowerShell.Management', 'Microsoft.PowerShell.Utility')) {
    Import-Module (Join-Path $PSHOME "Modules\$moduleName\$moduleName.psd1") -ErrorAction Stop
}
$repo = 'Azec-Zuckerberg/Azec-releases'
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
    if ($assets.Count -eq 1 -and $assets[0].digest -match '^sha256:[0-9a-fA-F]{64}$') { break }
    if ($attempt -lt 4) { Start-Sleep -Seconds 2 }
}
if ($release.tag_name -cne $tag -or $release.draft -or $release.prerelease) {
    throw 'The uploaded release is not the expected stable tag.'
}
if ($assets.Count -ne 1 -or $assets[0].name -cne 'Azec.exe') {
    throw 'The public release must contain only Azec.exe.'
}
$asset = $assets[0]
if ($asset.digest -notmatch '^sha256:([0-9a-fA-F]{64})$') { throw 'GitHub asset SHA-256 digest is missing.' }
$publishedHash = $Matches[1].ToLowerInvariant()
if ($publishedHash -cne $localHash) { throw 'GitHub asset digest differs from dist/Azec.exe.' }

$fixture = Join-Path $PSScriptRoot ('.build\verification\published-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $fixture -Force | Out-Null
$taggedExe = Join-Path $fixture 'tagged.exe'
$stableExe = Join-Path $fixture 'latest.exe'
# These requests deliberately carry no GitHub token: friends use anonymous downloads.
Invoke-WebRequest -UseBasicParsing -Uri "https://github.com/$repo/releases/download/$tag/Azec.exe" -OutFile $taggedExe -TimeoutSec 180 | Out-Null
if ((Get-FileHash -LiteralPath $taggedExe -Algorithm SHA256).Hash.ToLowerInvariant() -cne $publishedHash) {
    throw 'Downloaded tagged EXE differs from the asset digest.'
}
$latest = Invoke-RestMethod -Uri "https://api.github.com/repos/$repo/releases/latest" -Headers $headers -TimeoutSec 30
if ($latest.tag_name -cne $tag) { throw 'The stable latest release points to another version.' }
Invoke-WebRequest -UseBasicParsing -Uri "https://github.com/$repo/releases/latest/download/Azec.exe" -OutFile $stableExe -TimeoutSec 180 | Out-Null
if ((Get-FileHash -LiteralPath $stableExe -Algorithm SHA256).Hash.ToLowerInvariant() -cne $publishedHash) {
    throw 'The anonymous stable download differs from the published EXE.'
}
Write-Host "Verified $tag : local EXE, GitHub digest, tagged download, and anonymous latest download match."
