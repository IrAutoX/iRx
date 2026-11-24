@echo off
setlocal
set "VCPKG_ROOT=%VCPKG_INSTALLATION_ROOT%"
if "%VCPKG_ROOT%"=="" set "VCPKG_ROOT=%USERPROFILE%\vcpkg"
if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    echo vcpkg was not found. Set VCPKG_INSTALLATION_ROOT or install vcpkg.
    exit /b 1
)
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x86-windows
if errorlevel 1 exit /b %errorlevel%
cmake --build build --config Release --parallel
exit /b %errorlevel%
