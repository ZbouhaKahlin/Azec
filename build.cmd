@echo off
setlocal
cd /d "%~dp0"
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
"%NEON_QT_ROOT%\bin\qmake.exe" "%~dp0KingBoss2004.pro" -o Makefile
if errorlevel 1 exit /b 1
nmake /nologo
if errorlevel 1 exit /b 1
popd
if not exist runtime mkdir runtime
copy /y ".build\qt\release\KingBoss2004.exe" "runtime\KingBoss2004.exe" > nul
if errorlevel 1 exit /b 1
"%NEON_QT_ROOT%\bin\windeployqt.exe" --release --no-translations --no-opengl-sw --no-system-d3d-compiler --no-compiler-runtime --skip-plugin-types generic,networkinformation,tls,styles --dir "%~dp0runtime" "%~dp0runtime\KingBoss2004.exe"
if errorlevel 1 exit /b 1
pushd .build
cl /nologo /std:c++17 /O2 /EHsc /MT /DUNICODE /D_UNICODE ..\src\launcher.cpp /Fe:"..\KingBoss2004.exe" /link /SUBSYSTEM:WINDOWS user32.lib
if errorlevel 1 exit /b 1
if /I "%~1"=="test" (
    cl /nologo /std:c++17 /O2 /EHsc /MT /D_CRT_SECURE_NO_WARNINGS ..\src\core_test.cpp ..\src\core.cpp /Fe:core-tests.exe
    if errorlevel 1 exit /b 1
    .\core-tests.exe test-config.bin
    if errorlevel 1 exit /b 1
)
popd
echo Build complete: KingBoss2004.exe
