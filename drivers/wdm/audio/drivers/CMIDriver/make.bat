@echo off
call envars.bat
if "%DDKBUILDENV%"=="chk" goto :AlreadyDefined
call %CMI_DDKDIR%\bin\setenv %CMI_DDKDIR% chk %CMI_DEBUGARCH% %CMI_DEBUGOS%
:AlreadyDefined
cd %CMI_BUILDDIR%

if "%CMI_DEBUGARCH%"=="AMD64" goto :x64
set CMI_DEBUGDIR=objchk_%CMI_DEBUGOS%_%CMI_DEBUGARCH%\i386
mkdir %CMI_DEBUGDIR%
sed -e "s/CMIVersion/%CMI_VERSION%-dbg/" -e "s/CMIReleaseDate/%CMI_RELEASEDATE%/" CM8738-x32%WAVERTSTR%.INF >%CMI_DEBUGDIR%\CM8738.inf
goto start
:x64
set CMI_DEBUGDIR=objchk_%CMI_DEBUGOS%_%CMI_DEBUGARCH%\AMD64
mkdir %CMI_DEBUGDIR%
sed -e "s/CMIVersion/%CMI_VERSION%-dbg/" -e "s/CMIReleaseDate/%CMI_RELEASEDATE%/" CM8738-x64%WAVERTSTR%.inf >%CMI_DEBUGDIR%\CM8738.inf

:start
del %CMI_DEBUGDIR%\*.obj

sed -i "s/CMIVERSION.*/CMIVERSION \"%CMI_VERSION%-debug\"/" debug.hpp

if "%CMI_DEBUGVER%"=="WaveRT" goto :WaveRT
sed -i "s/^#define WAVERT/\/\/#define WAVERT/" debug.hpp
goto next
:WaveRT
sed -i "s/^\/\/#define WAVERT/#define WAVERT/" debug.hpp
:next

nmake /x errors.err

if "%CMI_DEBUGVER%"=="WaveRT" goto :WaveRT2
set WAVERTSTR=""
goto end
:WaveRT2
set WAVERTSTR="-WAVERT"

:end
sed -i "s/^cmicpl.*$//g" %CMI_DEBUGDIR%\CM8738.inf
sed -i "s/^CMICONTROL.*$//g" %CMI_DEBUGDIR%\CM8738.inf
