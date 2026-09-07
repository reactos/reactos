@echo off

::
::  Run an tf command on a list of files provided as a text file
::

set _Prog=%0

if "%3"=="" goto :Usage
call :ParseArgs %*

if "%TfCmd%"=="edit" (
	set TfExe=tf
	set TfCmd=edit /r
) else if "%TfCmd%"=="revert" (
	set TfExe=tfpt
	set TfCmd=uu /noget
) else (
	echo Unsupported command "%TfCmd%"
	goto :eof
)

setlocal enabledelayedexpansion
set filespecs=
for /f %%a in (%FileList%) do (
	if not "!filespecs!"=="" set filespecs=!filespecs! 
	set filespecs=!filespecs!%1\%%a
)

set command=%TfExe% %TfCmd% %filespecs%
echo %command%
%command%
goto :EOF

:Usage
	echo Usage: %_Prog% ^<path to wpf dir^> ^<text file with file list^> ^<tfcommand^>
	exit /b 1
	goto :EOF

:ParseArgs
	set WindowsDir=%1
	set FileList=%2
	set TfCmd=%3
	goto :EOF
