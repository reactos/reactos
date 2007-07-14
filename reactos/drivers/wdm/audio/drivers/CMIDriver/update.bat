@echo off
call envars.bat
if "%CMI_DEBUGARCH%"=="AMD64" goto :x64
set CMI_DEBUGDIR=objchk_%CMI_DEBUGOS%_%CMI_DEBUGARCH%\i386
goto start
:x64
set CMI_DEBUGDIR=objchk_%CMI_DEBUGOS%_%CMI_DEBUGARCH%\AMD64
:start
devcon remove "PCI\VEN_13F6&DEV_0111&SUBSYS_011113F6&REV_10"
devcon dp_delete %CMI_OEMINF%
devcon rescan
devcon update %CMI_DEBUGDIR%\CM8738.INF "PCI\VEN_13F6&DEV_0111&SUBSYS_011113F6&REV_10"
