@echo off
setlocal
cd /d "%~dp0"
set "BUILD_CHECKS="
if /I "%~1"=="checks" set "BUILD_CHECKS=1"
if /I "%~1"=="test" set "BUILD_CHECKS=1"
if not exist .build mkdir .build
if not defined NEON_QT_ROOT set "NEON_QT_ROOT=%LOCALAPPDATA%\NeonManagerR6\Qt\6.8.3\msvc2022_64"
if not exist "%NEON_QT_ROOT%\bin\qmake.exe" (
    echo Qt 6.8.3 is missing. Run setup-qt.ps1 first, or set NEON_QT_ROOT.
    exit /b 1
)
"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > .build\visual-studio-path.txt
set "R6_VS="
set /p R6_VS=<.build\visual-studio-path.txt
if not defined R6_VS exit /b 1
set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
call "%R6_VS%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 -startdir=none
if errorlevel 1 exit /b 1
cd /d "%~dp0"
set "PATH=%NEON_QT_ROOT%\bin;%PATH%"
if not exist .build\qt mkdir .build\qt
pushd .build\qt
"%NEON_QT_ROOT%\bin\qmake.exe" "%~dp0Azec.pro" -o Makefile
if errorlevel 1 exit /b 1
nmake /nologo
if errorlevel 1 exit /b 1
popd
if not exist runtime mkdir runtime
copy /y ".build\qt\release\Azec.exe" "runtime\Azec.exe" > nul
if errorlevel 1 exit /b 1
"%NEON_QT_ROOT%\bin\windeployqt.exe" --release --no-translations --no-opengl-sw --no-system-d3d-compiler --no-compiler-runtime --skip-plugin-types generic,networkinformation,styles --dir "%~dp0runtime" "%~dp0runtime\Azec.exe"
if errorlevel 1 exit /b 1
rem Bundle the Visual C++ runtime beside the app so a clean Windows install needs no redistributable.
set "R6_CRT="
for /d %%D in ("%VCToolsRedistDir%x64\Microsoft.VC*.CRT") do set "R6_CRT=%%~fD"
if not defined R6_CRT (
    echo Visual C++ redistributable files were not found under "%VCToolsRedistDir%".
    exit /b 1
)
copy /y "%R6_CRT%\*.dll" "%~dp0runtime\" > nul
if errorlevel 1 exit /b 1
pushd .build
rc /nologo /I "..\assets" /fo azec-icon.res "..\assets\app.rc"
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /O2 /EHsc /MT /DUNICODE /D_UNICODE ..\src\launcher.cpp azec-icon.res /Fe:"..\Azec.exe" /link /SUBSYSTEM:WINDOWS user32.lib shell32.lib ole32.lib uuid.lib
if errorlevel 1 exit /b 1
if /I "%~1"=="test" (
    cl /nologo /std:c++17 /O2 /EHsc /MT /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS ..\tests\core_test.cpp ..\tests\core_checks.cpp ..\src\core.cpp /Fe:core-tests.exe
    if errorlevel 1 exit /b 1
    .\core-tests.exe test-config.bin
    if errorlevel 1 exit /b 1
)
popd
if defined BUILD_CHECKS (
    if not exist .build\verification-build mkdir .build\verification-build
    pushd .build\verification-build
    "%NEON_QT_ROOT%\bin\qmake.exe" "%~dp0Azec.pro" CONFIG+=verification -o Makefile
    if errorlevel 1 exit /b 1
    nmake /nologo
    if errorlevel 1 exit /b 1
    popd
    copy /y ".build\verification-build\release\Azec-checks.exe" "runtime\Azec-checks.exe" > nul
    if errorlevel 1 exit /b 1
)
if /I "%~1"=="test" (
    python tests\run_checks.py %2
    if errorlevel 1 exit /b 1
)
echo Build complete: Azec.exe
