param(
    [string]$Output = "dist/Azec-portable",
    [string]$Zip = "dist/Azec-portable.zip"
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

$distRoot = [IO.Path]::GetFullPath((Join-Path $root 'dist')) + [IO.Path]::DirectorySeparatorChar
$Output = [IO.Path]::GetFullPath($(if ([IO.Path]::IsPathRooted($Output)) { $Output } else { Join-Path $root $Output }))
$Zip = [IO.Path]::GetFullPath($(if ([IO.Path]::IsPathRooted($Zip)) { $Zip } else { Join-Path $root $Zip }))
foreach ($destination in @($Output, $Zip)) {
    if (-not $destination.StartsWith($distRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Package outputs must be inside this checkout''s dist folder.'
    }
    for ($ancestor = $destination; $ancestor -and $ancestor.StartsWith($root, [StringComparison]::OrdinalIgnoreCase); $ancestor = Split-Path -Parent $ancestor) {
        if ((Test-Path -LiteralPath $ancestor) -and ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Refusing to write a package through a linked path: $ancestor"
        }
    }
}
if ($Zip.Equals($Output, [StringComparison]::OrdinalIgnoreCase) -or $Zip.StartsWith($Output + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'The ZIP must be outside the package folder.'
}
if (Test-Path -LiteralPath $Output) { Remove-Item -LiteralPath $Output -Recurse -Force }
if (Test-Path -LiteralPath $Zip) { Remove-Item -LiteralPath $Zip -Force }
New-Item -ItemType Directory -Path $Output | Out-Null

Copy-Item -LiteralPath 'Azec.exe' -Destination $Output
Copy-Item -LiteralPath 'Run.cmd' -Destination $Output
Copy-Item -LiteralPath 'README.md' -Destination $Output
Copy-Item -LiteralPath 'LICENSE' -Destination $Output
Copy-Item -LiteralPath 'licenses' -Destination (Join-Path $Output 'licenses') -Recurse

$runtime = Join-Path $Output 'runtime'
New-Item -ItemType Directory -Path $runtime | Out-Null
# Copy runtime components only. Development settings, driver downloads, capture
# folders, old builds, and verification binaries stay in the working checkout.
Get-ChildItem -LiteralPath 'runtime' -File | Where-Object {
    $_.Name -eq 'Azec.exe' -or $_.Name -like 'Qt6*.dll' -or
    $_.Name -like 'msvcp140*.dll' -or $_.Name -like 'vcruntime140*.dll' -or
    $_.Name -in @('concrt140.dll', 'vccorlib140.dll', 'ucrtbase.dll', 'dxcompiler.dll', 'dxil.dll', 'opengl32sw.dll') -or
    $_.Name -like 'api-ms-win-*.dll' -or $_.Name -like 'd3dcompiler_*.dll'
} | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $runtime }
foreach ($plugin in @('platforms', 'tls', 'imageformats', 'iconengines', 'styles', 'networkinformation', 'generic')) {
    $source = Join-Path 'runtime' $plugin
    if (Test-Path -LiteralPath $source) {
        $target = Join-Path $runtime $plugin
        New-Item -ItemType Directory -Path $target | Out-Null
        Get-ChildItem -LiteralPath $source -File -Filter '*.dll' | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $target }
    }
}

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
    'runtime\tls\qschannelbackend.dll',
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

3. Recoil starts switched OFF. Rapid fire follows each saved weapon profile.
   Use the header or tray to control output. Press Insert to show or hide.

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
