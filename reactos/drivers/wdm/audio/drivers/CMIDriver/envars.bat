@echo off
set CMI_DDKDIR=c:\WinDDK\6000
set CMI_BUILDDIR=C:\WinDDK\6000\src\Audio\CMedia
set CMI_VERSION=1.1.3
REM the slashes need to be escaped for sed.exe
set CMI_RELEASEDATE=06\/30\/2007
REM wxp | wlh
set CMI_DEBUGOS=wlh
REM x86 | AMD64
set CMI_DEBUGARCH=x86
REM WaveRT | 
set CMI_DEBUGVER=
REM find out with 'devcon dp_enum'
set CMI_OEMINF="oem1.inf"