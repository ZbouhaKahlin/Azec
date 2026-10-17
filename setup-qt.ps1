$ErrorActionPreference='Stop'
$neonTools=Join-Path $env:LOCALAPPDATA 'NeonManagerR6\build-tools'
$neonQt=Join-Path $env:LOCALAPPDATA 'NeonManagerR6\Qt'
python -m pip install --disable-pip-version-check --target $neonTools 'aqtinstall==3.3.0'
if ($LASTEXITCODE -ne 0) { throw 'Build-tool installation failed' }
$env:PYTHONPATH=$neonTools
python -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 --archives qtbase qtsvg --outputdir $neonQt
if ($LASTEXITCODE -ne 0) { throw 'Qt installation failed' }
