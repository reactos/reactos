@echo off
REM
REM This batch script will mark an image file so that it thinks it is running
REM on Windows 95.
REM
if "%1" == "" goto usage
echo Marking %1 executable so it thinks it is running on Windows 95
imagecfg -w 0xC0000004 %1 >nul
goto done
:usage
echo Usage: SETWIN95 executableFileName
:done
