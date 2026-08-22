@echo off

rem Test of csp.exe for a regular C# project.

rem csp.exe is easy to run. This script just:
rem
rem 1) Uses csp.exe from where it's built.
rem 2) Reports a useful message if csp.exe hasn't been built
rem 3) Makes sure we're in the right directory


rem Doesn't use a .rsp file or pass parameters to the project
rem (see test\CsProject for an example of that).



setlocal enabledelayedexpansion

rem 
rem       calculate this.
rem       Candidates: build.hostarch build_hostarch HOST_PROCESSOR_ARCHITECTURE
rem                   PROCESSOR_ARCHITECTURE
set hostarch=i386

pushd %~dp0
set TOOLSRCPATH=..\..
set TOOLPATH=%TOOLSRCPATH%\obj%BUILD_ALT_DIR%\%hostarch%

if not exist %TOOLPATH%\Csp.exe call :needToBuild %TOOLSRCPATH% & goto:end

%TOOLPATH%\Csp -s:CsProjectTest.cs
goto :end

:needToBuild
echo RunTest.cmd(0) : error : Csp.exe not found (need to build in %~dp1)
goto :eof


:end
popd

