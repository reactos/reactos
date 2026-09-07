@echo off

rem Test of csp.exe for a "C# prime" project.
rem
rem Also tests:
rem   * Parameters passed
rem   * Use of a .rsp file
rem   * "-main" parameter

setlocal

rem 
rem       calculate this.
rem       Candidates: build.hostarch build_hostarch HOST_PROCESSOR_ARCHITECTURE
rem                   PROCESSOR_ARCHITECTURE
set hostarch=i386

pushd %~dp0
set TOOLSRCPATH=..\..
set TOOLPATH=%TOOLSRCPATH%\obj%BUILD_ALT_DIR%\%hostarch%

if not exist %TOOLPATH%\Csp.exe call :needToBuild %TOOLSRCPATH% & goto:end

%TOOLPATH%\Csp -enablecsprime -rsp:CspProject.rsp -- testparam1 testparam2
goto :end

:needToBuild
echo RunTest.cmd(0) : error : Csp.exe not found (need to build in %~dp1)
goto :eof


:end
popd

