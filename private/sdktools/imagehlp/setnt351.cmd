@echo off
REM
REM This batch script will mark an image file so that it thinks it is running
REM on Windows NT 3.51
REM
if "%1" == "" goto usage
echo Marking %1 executable so it thinks it is running on Windows NT 3.51
imagecfg -w 0x04213303 %1 >nul
goto done
:usage
echo Usage: SETNT351 executableFileName
:done
