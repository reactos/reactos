@echo off

:: Get the source root directory
set REACTOS_SOURCE_DIR=%~dp0

:: Detect build environment (MinGW, VS, WDK, ...)
if defined ROS_ARCH (
    set BUILD_ENVIRONMENT=MinGW
    set ARCH=%ROS_ARCH%
    echo Detected RosBE for %ROS_ARCH%
) else if defined DDK_TARGET_OS (
    if "%_BUILDARCH%" == "x86" (
        set ARCH=i386
    )
    if "%_BUILDARCH%" == "AMD64" (
        set ARCH=amd64
    )
    set BUILD_ENVIRONMENT=WDK
    echo Detected DDK/WDK for %DDK_TARGET_OS%-%ARCH%
)else if defined VCINSTALLDIR (
:: VS command prompt does not put this in enviroment vars
    cl 2>&1 | find "x86" > NUL && set ARCH=i386
    cl 2>&1 | find "x64" > NUL && set ARCH=amd64
    cl 2>&1 | find "14." > NUL && set BUILD_ENVIRONMENT=VS8
    cl 2>&1 | find "15." > NUL && set BUILD_ENVIRONMENT=VS9
    cl 2>&1 | find "16." > NUL && set BUILD_ENVIRONMENT=VS10
    if not defined BUILD_ENVIRONMENT (
        echo Error: Visual Studio version too old or version detection failed.
        exit /b	
    )
    echo Detected Visual Studio Environment %BUILD_ENVIRONMENT%-%ARCH%
) else if defined sdkdir (
    if "%TARGET_CPU%" == "x86" (
        set ARCH=i386
    )
    if "%TARGET_CPU%" == "x64" (
        set ARCH=amd64
    )
    set BUILD_ENVIRONMENT=SDK
    echo Detected Windows SDK %TARGET_PLATFORM%-%ARCH%
)

if defined ARCH if defined BUILD_ENVIRONMENT (
    goto createdirs
)

echo Error: Critical variable missing. Configure script failure.
exit /b

:: Create directories
:createdirs

set REACTOS_OUTPUT_PATH=output-%BUILD_ENVIRONMENT%-%ARCH% 
if "%REACTOS_SOURCE_DIR%" == "%CD%" (
    echo test
    echo Creating directories in %REACTOS_OUTPUT_PATH%

    if not exist %REACTOS_OUTPUT_PATH% (
        mkdir %REACTOS_OUTPUT_PATH%
    )
    cd %REACTOS_OUTPUT_PATH%
)

if not exist host-tools (
    mkdir host-tools
)
if not exist reactos (
    mkdir reactos
)

:hostprep
echo Preparing host tools...
cd host-tools
if EXIST CMakeCache.txt (
    del CMakeCache.txt /q
)
set REACTOS_BUILD_TOOLS_DIR=%CD%

if "%BUILD_ENVIRONMENT%" == "MinGW" (
    cmake -G "MinGW Makefiles" -DARCH=%ARCH% %REACTOS_SOURCE_DIR%
) else if "%BUILD_ENVIRONMENT%" == "WDK" (
    cmake -G "NMake Makefiles" -DARCH=%ARCH% %REACTOS_SOURCE_DIR%
) else if "%BUILD_ENVIRONMENT%" == "SDK" (
    cmake -G "NMake Makefiles" -DARCH=%ARCH% %REACTOS_SOURCE_DIR%
) else if "%BUILD_ENVIRONMENT%" == "VS8" (
    if "%ARCH%" == "amd64" (
        cmake -G "Visual Studio 8 2005 Win64" -DARCH=%ARCH% %REACTOS_SOURCE_DIR%
    ) else (
        cmake -G "Visual Studio 8 2005" -DARCH=%ARCH% %REACTOS_SOURCE_DIR%
    )
) else if "%BUILD_ENVIRONMENT%" == "VS9" (
    if "%ARCH%" == "amd64" (
        cmake -G "Visual Studio 9 2008 Win64" -DARCH=%ARCH% %REACTOS_SOURCE_DIR%
    ) else (
        cmake -G "Visual Studio 9 2008" -DARCH=%ARCH% %REACTOS_SOURCE_DIR%
    )
) else if "%BUILD_ENVIRONMENT%" == "VS10" (
    if "%ARCH%" == "amd64" (
        cmake -G "Visual Studio 10 Win64" -DARCH=%ARCH% %REACTOS_SOURCE_DIR%
    ) else (
        cmake -G "Visual Studio 10" -DARCH=%ARCH% %REACTOS_SOURCE_DIR%
    )
)
cd..

:reactprep
echo Preparing reactos...

cd reactos
if EXIST CMakeCache.txt (
    del CMakeCache.txt /q
)

if "%BUILD_ENVIRONMENT%" == "MinGW" (
    cmake -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw32.cmake -DARCH=%ARCH% -DREACTOS_BUILD_TOOLS_DIR:DIR="%REACTOS_BUILD_TOOLS_DIR%" %REACTOS_SOURCE_DIR%
) else if "%BUILD_ENVIRONMENT%" == "WDK" (
    cmake -G "NMake Makefiles" -DCMAKE_TOOLCHAIN_FILE=toolchain-msvc.cmake -DARCH=%ARCH% -DREACTOS_BUILD_TOOLS_DIR:DIR="%REACTOS_BUILD_TOOLS_DIR%" %REACTOS_SOURCE_DIR%
) else if "%BUILD_ENVIRONMENT%" == "SDK" (
    cmake -G "NMake Makefiles" -DCMAKE_TOOLCHAIN_FILE=toolchain-msvc.cmake -DARCH=%ARCH% -DREACTOS_BUILD_TOOLS_DIR:DIR="%REACTOS_BUILD_TOOLS_DIR%" %REACTOS_SOURCE_DIR%
) else if "%BUILD_ENVIRONMENT%" == "VS8" (
    if "%ARCH%" == "amd64" (
        cmake -G "Visual Studio 8 2005 Win64" -DCMAKE_TOOLCHAIN_FILE=toolchain-msvc.cmake -DARCH=%ARCH% -DREACTOS_BUILD_TOOLS_DIR:DIR="%REACTOS_BUILD_TOOLS_DIR%" %REACTOS_SOURCE_DIR%
    ) else (
        cmake -G "Visual Studio 8 2005" -DCMAKE_TOOLCHAIN_FILE=toolchain-msvc.cmake -DARCH=%ARCH% -DREACTOS_BUILD_TOOLS_DIR:DIR="%REACTOS_BUILD_TOOLS_DIR%" %REACTOS_SOURCE_DIR%
    )
) else if "%BUILD_ENVIRONMENT%" == "VS9" (
    if "%ARCH%" == "amd64" (
        cmake -G "Visual Studio 9 2008 Win64" -DCMAKE_TOOLCHAIN_FILE=toolchain-msvc.cmake -DARCH=%ARCH% -DREACTOS_BUILD_TOOLS_DIR:DIR="%REACTOS_BUILD_TOOLS_DIR%" %REACTOS_SOURCE_DIR%
    ) else (
        cmake -G "Visual Studio 9 2008" -DCMAKE_TOOLCHAIN_FILE=toolchain-msvc.cmake -DARCH=%ARCH% -DREACTOS_BUILD_TOOLS_DIR:DIR="%REACTOS_BUILD_TOOLS_DIR%" %REACTOS_SOURCE_DIR%
    )
) else if "%BUILD_ENVIRONMENT%" == "VS10" (
    if "%ARCH%" == "amd64" (
        cmake -G "Visual Studio 10 Win64" -DCMAKE_TOOLCHAIN_FILE=toolchain-msvc.cmake -DARCH=%ARCH% -DREACTOS_BUILD_TOOLS_DIR:DIR="%REACTOS_BUILD_TOOLS_DIR%" %REACTOS_SOURCE_DIR%
    ) else (
        cmake -G "Visual Studio 10" -DCMAKE_TOOLCHAIN_FILE=toolchain-msvc.cmake -DARCH=%ARCH% -DREACTOS_BUILD_TOOLS_DIR:DIR="%REACTOS_BUILD_TOOLS_DIR%" %REACTOS_SOURCE_DIR%
    )
)

cd..
if not ERRORLEVEL  == 0 (
    echo Warning: errors occured.
)

echo Configure script complete! Enter directories and execute appropriate build commands(ex: make, nmake, etc...).

