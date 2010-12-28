@echo off

rem Get the source root directory
set ROS_SOURCE_DIR=%~dp0

rem Detect build environment (Mingw, VS, WDK, ...)
if not "%ROS_ARCH%" == "" (
    echo Detected RosBE for %ROS_ARCH%
    set BUILD_ENVIRONMENT=MINGW
)
if not "%DDK_TARGET_OS%" == "" (
    echo Detected DDK/WDK for %_BUILDARCH%
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
	cmake -G "MinGW Makefiles" -DARCH=%ROS_ARCH% %ROS_SOURCE_DIR%
)
if "%BUILD_ENVIRONMENT%" == "WDK" (
	cmake -G "NMake Makefiles" -DARCH=%_BUILDARCH% %ROS_SOURCE_DIR%
)
cd..

echo Preparing reactos...
if not exist reactos (
    mkdir reactos
)

cd reactos
if "%BUILD_ENVIRONMENT%" == "MINGW" (
    cmake -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw32.cmake -DARCH=%ROS_ARCH% -DREACTOS_BUILD_TOOLS_DIR:DIR="%REACTOS_BUILD_TOOLS_DIR%" %ROS_SOURCE_DIR%
)
if "%BUILD_ENVIRONMENT%" == "WDK" (
    cmake -G "NMake Makefiles" -DCMAKE_TOOLCHAIN_FILE=toolchain-msvc.cmake -DARCH=%_BUILDARCH% -DREACTOS_BUILD_TOOLS_DIR:DIR="%REACTOS_BUILD_TOOLS_DIR%" %ROS_SOURCE_DIR%
)
cd..

rem Create a root makefile
rem echo ... > makefile
