@echo off

:: This is a helper script to invoke CSP.exe.  It warns the user if CSP.exe hasn't
:: been built and runs the exe in the preferred way under razzle.  Arguements to
:: CSP are passed on the command line.

setlocal enabledelayedexpansion

set HostArch=%_BuildArch%
if "%HostArch%"=="x86" set HostArch=i386
set HostArch=i386

set CspSrcPath=%_NtBinDir%\wpf\src\Graphics\tools\csp
set CspExePath=%_NTTREE%\wpf
@echo %CspExePath%

if not exist %CspExePath%\csp.exe echo GenerateFiles.cmd(0) : error : csp.exe not found (need to build in %CspSrcPath%)& goto :eof

:: Run from parent directory (relative to GenerateFiles.cmd)
pushd %~dp0\..

call %~dp0\SetClrPath.cmd

copy %CspExePath%\csp.exe %clrpath%
copy csp.exe.config %clrpath%
copy csp.exe.manifest %clrpath%

:: Register for skipping strong-name validation for CSP
:: We don't unregister at the end in case somebody is using CSP separately and
:: has registered separately.
%clrpath%\sn.exe -Vr %clrpath%\csp.exe

:: Execute the MilCodeGen project
%DebuggerHook% %clrpath%\csp.exe %*

:: It's really annoying when csp.exe fails, but doesn't emit an error message.
:: This line ensures that an error message will be emitted when csp.exe fails.
:: The "invokeCSP.cmd(0) : error : " part is necessary for the build.exe to
:: emit the error to the console. Otherwise, it just swallows the error.
if NOT ERRORLEVEL 0 echo invokeCSP.cmd(0) : error : csp failed. Check log. 1>&2

popd
endlocal
goto :EOF
