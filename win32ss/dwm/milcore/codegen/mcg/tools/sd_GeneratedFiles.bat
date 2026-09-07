@echo off

::
::  Run an sd command on a list of files provided as a text file
::

set _Prog=%0

if "%3"=="" goto :Usage
call :ParseArgs %*

if "%SdCmd%"=="edit" goto :ok
if "%SdCmd%"=="revert" goto :ok
echo Unsupported command "%SdCmd%"
goto :eof

:ok
setlocal enabledelayedexpansion
set filespecs=
for /f %%a in (%FileList%) do (
	if not "!filespecs!"=="" set filespecs=!filespecs! 
	set filespecs=!filespecs!%1\%%a
)

set command=sd %SdCmd% %SdArgs% %filespecs%
echo %command%
%command%
goto :EOF

:Usage
	echo Usage: %_Prog% ^<path to wpf dir^> ^<text file with file list^> ^<sdcommand^> [^<params to sd^>]
	exit /b 1
	goto :EOF

:ParseArgs
	set WindowsDir=%1
	set FileList=%2
	set SdCmd=%3
	set SdArgs=%4 %5 %6 %7 %8 %9
	goto :EOF
