@echo off

rem Get the source root directory
set ROS_SOURCE_DIR=%~dp0

rem Detect build environment (Mingw, VS, WDK, ...)
if "%ROS_ARCH%" == "i386" (
    echo Detected RosBE for i386
    set BUILD_ENVIRONMENT=MINGW
)
if "%ROS_ARCH%" == "amd64" (
    echo Detected RosBE for amd64
    set BUILD_ENVIRONMENT=MINGW
)
if "%ROS_ARCH%" == "arm" (
    echo Detected RosBE for arm
    set BUILD_ENVIRONMENT=MINGW
)
if not "%DDK_TARGET_OS%" == "" (
    echo Detected DDK/WDK
    set BUILD_ENVIRONMENT=WDK
)



rem Create directories
echo Preparing host tools...
if not exist host-tools (
    mkdir host-tools
)
cd host-tools
set REACTOS_BUILD_TOOLS_DIR=%CD%
if "%BUILD_ENVIRONMENT%" == "MINGW" (
	cmake -G "MinGW Makefiles" %ROS_SOURCE_DIR%
)
if "%BUILD_ENVIRONMENT%" == "WDK" (
	cmake -G "NMake Makefiles" %ROS_SOURCE_DIR%
)
cd..

echo Preparing reactos...
if not exist reactos (
    mkdir reactos
)

cd reactos
if "%BUILD_ENVIRONMENT%" == "MINGW" (
	cmake -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw32.cmake %ROS_SOURCE_DIR% -DREACTOS_BUILD_TOOLS_DIR:DIR="%REACTOS_BUILD_TOOLS_DIR%"
)
if "%BUILD_ENVIRONMENT%" == "WDK" (
	cmake -G "NMake Makefiles" -DCMAKE_TOOLCHAIN_FILE=toolchain-msc.cmake %ROS_SOURCE_DIR% -DREACTOS_BUILD_TOOLS_DIR:DIR="%REACTOS_BUILD_TOOLS_DIR%"
)
cd..

rem Create a root makefile
@echo someshit > makefile
