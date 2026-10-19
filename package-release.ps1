param(
    [string]$Output = "dist/Azec-portable",
    [string]$Zip = "dist/Azec-portable.zip"
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

if (Test-Path $Output) { Remove-Item $Output -Recurse -Force }
if (Test-Path $Zip) { Remove-Item $Zip -Force }
New-Item -ItemType Directory -Path $Output | Out-Null

Copy-Item -LiteralPath 'Azec.exe' -Destination $Output
Copy-Item -LiteralPath 'Run.cmd' -Destination $Output
Copy-Item -LiteralPath 'README.md' -Destination $Output
Copy-Item -LiteralPath 'runtime' -Destination (Join-Path $Output 'runtime') -Recurse
Copy-Item -LiteralPath 'licenses' -Destination (Join-Path $Output 'licenses') -Recurse

$runtime = Join-Path $Output 'runtime'
Get-ChildItem $runtime -File | Where-Object {
    $_.Name -like 'Azec-checks.exe' -or
    $_.Name -like 'Azec.exe.old-*' -or
    $_.Name -like 'Azec.exe.pre-fix-*' -or
    $_.Name -like '*.tmp-*' -or
    $_.Name -like '*.log'
} | Remove-Item -Force

# A friend's clean PC has neither Qt nor the Visual C++ runtime installed, so fail
# loudly instead of shipping a package that shows a missing-DLL dialog.
$required = @(
    'runtime\Azec.exe',
    'runtime\Qt6Core.dll',
    'runtime\Qt6Gui.dll',
    'runtime\Qt6Widgets.dll',
    'runtime\Qt6Network.dll',
    'runtime\Qt6Svg.dll',
    'runtime\platforms\qwindows.dll',
    'runtime\vcruntime140.dll',
    'runtime\vcruntime140_1.dll',
    'runtime\msvcp140.dll',
    'runtime\msvcp140_1.dll',
    'runtime\msvcp140_2.dll'
)
$missing = $required | Where-Object { -not (Test-Path (Join-Path $Output $_)) }
if ($missing) {
    throw "Package is incomplete. Run build.cmd first. Missing: $($missing -join ', ')"
}

$startHere = @'
Azec - how to run
=================

1. Extract this whole .zip into a folder (right-click the zip, Extract All).
   Start Azec from that folder, not from inside the zip window.

2. Open Azec.exe. Keep runtime\ and licenses\ beside it - they are required.
   Nothing is installed and no admin rights are needed. To remove Azec,
   delete the folder.

3. Recoil and Rapid fire start switched OFF. Turn on what you use from the
   header (or the tray icon). Press Insert to show or hide the window.

Windows 10 or 11, 64-bit. If Windows shows "Windows protected your PC",
click More info then Run anyway: the app is not code-signed.

The phone page and profiles work offline over your local network. Recoil and
rapid fire are macros: use them at your own risk and follow the game's rules.
'@
Set-Content -LiteralPath (Join-Path $Output 'START-HERE.txt') -Value $startHere -Encoding ASCII

# Never package personal configuration, pairing data, or captures.
$forbidden = @('r6_config.bin','azec_weapons.json','azec_ui.json','azec.log','phone-info.json','ui-verification.json')
Get-ChildItem $Output -Recurse -File | Where-Object { $forbidden -contains $_.Name } | ForEach-Object {
    throw "Refusing to package personal file: $($_.FullName)"
}

# Archive the folder itself so extracting creates a tidy Azec-portable folder
# instead of spilling files into the Downloads directory.
Compress-Archive -Path $Output -DestinationPath $Zip -Force
$size = [math]::Round((Get-Item $Zip).Length / 1MB, 1)
Write-Host "Packaged $Zip ($size MB). Send that zip; the friend extracts it and runs Azec.exe."
