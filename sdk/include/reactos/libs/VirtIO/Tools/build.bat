@echo off
setlocal enabledelayedexpansion

rem ================================================================================
rem Virtio-win master build script
rem
rem Usage: build.bat <project_or_solution_file_path> <target_os_versions> [<args>]
rem
rem where args can be one or more of
rem   Debug, dbg, chk       .. to build Debug rather than the default release flavor
rem   amd64, x64, 64        .. to build only 64-bit driver
rem   x86, 32               .. to build only 32-bit driver
rem   /Option               .. build command to pass to VS, for example /Rebuild
rem   Win7, Win8, Win10..   .. target OS version
rem
rem By default the script performs an incremental build of both 32-bit and 64-bit
rem release drivers for all supported target OSes.
rem
rem To do a Static Driver Verifier build, append _SDV to the target OS version, for
rem example Win10_SDV.
rem ================================================================================

rem This is a list of supported build target specifications A_B where A is the
rem VS project configuration name and B is the corresponding platform identifier
rem used in log file names and intermediate directory names. Either of the two can
rem be used in the <target_os_version> command line argument.
set SUPPORTED_BUILD_SPECS=WinXP_wxp Win2k3_wnet Vista_wlh Win7_win7 Win8_win8 Win8.1_win81 Win10_win10

set BUILD_TARGETS=%~2
set BUILD_DIR=%~dp1
set BUILD_FILE=%~nx1

rem We do an incremental Release build for all specs and all archs by default
set BUILD_FLAVOR=Release
set BUILD_COMMAND=/Build
set BUILD_SPEC=
set BUILD_ARCH=
set BUILD_FAILED=

set VSFLAVOR=Professional
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\devenv.com" set VSFLAVOR=Community
echo USING !VSFLAVOR! Visual Studio

rem Parse arguments
:argloop
shift /2
if "%2"=="" goto argend
set ARG=%2
if "%ARG:~0,1%"=="/" set BUILD_COMMAND=%ARG%& goto :argloop

if /I "%ARG%"=="Debug" set BUILD_FLAVOR=Debug& goto :argloop
if /I "%ARG%"=="dbg" set BUILD_FLAVOR=Debug& goto :argloop
if /I "%ARG%"=="chk" set BUILD_FLAVOR=Debug& goto :argloop

if /I "%ARG%"=="amd64" set BUILD_ARCH=amd64& goto :argloop
if /I "%ARG%"=="64" set BUILD_ARCH=amd64& goto :argloop
if /I "%ARG%"=="x64" set BUILD_ARCH=amd64& goto :argloop
if /I "%ARG%"=="32" set BUILD_ARCH=x86& goto :argloop
if /I "%ARG%"=="x86" set BUILD_ARCH=x86& goto :argloop
if /I "%ARG%"=="ARM64" set BUILD_ARCH=ARM64& goto :argloop

rem Assume that this is target OS version and split off the tag
call :split_target_tag "%ARG%"

rem Verify that this target OS is supported and valid
for %%N in (%SUPPORTED_BUILD_SPECS%) do (
  set T=%%N
  set CANDIDATE_SPEC=
  set FOUND_MATCH=

  for %%A in ("!T:_=" "!") do (
    if /I %%A=="%TARGET%" set CANDIDATE_SPEC=!T!!TAG!
    for %%B in (%BUILD_TARGETS%) do (
      if /I %%B==%%~A!TAG! set FOUND_MATCH=1
    )
  )

  if not "!FOUND_MATCH!"=="" if not "!CANDIDATE_SPEC!"=="" (
    set BUILD_SPEC=!CANDIDATE_SPEC!
    goto :argloop
  )
)

rem Silently exit if the build target could not be matched
rem
rem The reason for ignoring build target mismatch are projects
rem like NetKVM, viostor, and vioscsi, which build different
rem sln/vcxproj for different targets. Higher level script
rem does not have information about specific sln/vcproj and
rem platform bindings, therefore it invokes this script once
rem for each sln/vcproj to make it decide when the actual build
rem should be invoked.

goto :eof

rem Figure out which targets we're building
:argend
if "%BUILD_SPEC%"=="" (
  for %%B in (%BUILD_TARGETS%) do (
    call :split_target_tag "%%B"
    for %%N in (%SUPPORTED_BUILD_SPECS%) do (
      set T=%%N
      set BUILD_SPEC=
      for %%A in ("!T:_=" "!") do (
        if /I %%A=="!TARGET!" set BUILD_SPEC=!T!!TAG!
      )
      if not "!BUILD_SPEC!"=="" (
        call :build_target !BUILD_SPEC!
        if not "!BUILD_FAILED!"=="" goto :fail
      )
    )
  )
) else (
  call :build_target %BUILD_SPEC%
)
goto :eof

rem Figure out which archs we're building
:build_target
if "%BUILD_ARCH%"=="" (
  call :build_arch %1 x86
  if not "!BUILD_FAILED!"=="" goto :eof
  call :build_arch %1 amd64
) else (
  call :build_arch %1 %BUILD_ARCH%
)
goto :eof

rem Invoke Visual Studio
:build_arch
setlocal
set BUILD_ARCH=%2
set TAG=
for /f "tokens=1 delims=_" %%T in ("%1") do (
  set TARGET_PROJ_CONFIG=%%T
)
for /f "tokens=2 delims=_" %%T in ("%1") do (
  set TARGET_PLATFORM=%%T
)
for /f "tokens=3 delims=_" %%T in ("%1") do (
  set TAG=%%T
)

rem There is no 64-bit XP build
if /I "%1"=="WinXP_wxp" if /I "%BUILD_ARCH%"=="amd64" goto :eof

if /I "!TAG!"=="SDV" (
  rem There is no 32-bit SDV build
  if %BUILD_ARCH%==x86 goto :eof
  rem Check the SDV build suppression variable
  if not "%_BUILD_DISABLE_SDV%"=="" (
    echo Skipping %TARGET_PROJ_CONFIG% SDV build because _BUILD_DISABLE_SDV is set
    goto :eof
  )
)

rem Compose build log file name
if %BUILD_FLAVOR%=="Debug" (
  set BUILD_LOG_FILE=buildchk
) else (
  set BUILD_LOG_FILE=buildfre
)
set BUILD_LOG_FILE=%BUILD_LOG_FILE%_%TARGET_PLATFORM%_%BUILD_ARCH%.log

if %BUILD_ARCH%==amd64 set BUILD_ARCH=x64
set TARGET_VS_CONFIG="%TARGET_PROJ_CONFIG% %BUILD_FLAVOR%|%BUILD_ARCH%"

pushd %BUILD_DIR%
call "%~dp0\SetVsEnv.bat" x86

if /I "!TAG!"=="SDV" (
  echo Running SDV for %BUILD_FILE%, configuration %TARGET_VS_CONFIG%
  call :runsdv "%TARGET_PROJ_CONFIG% %BUILD_FLAVOR%" %BUILD_ARCH%
) else (
  echo Building %BUILD_FILE%, configuration %TARGET_VS_CONFIG%, command %BUILD_COMMAND%
  call "C:\Program Files (x86)\Microsoft Visual Studio\2017\!VSFLAVOR!\Common7\IDE\devenv.com" %BUILD_FILE% %BUILD_COMMAND% %TARGET_VS_CONFIG% /Out %BUILD_LOG_FILE%
)
popd
endlocal

IF ERRORLEVEL 1 (
  set BUILD_FAILED=1
)
goto :eof

:runsdv
set SDVLEGACY=1
call "%~dp0\SetVsEnv.bat" x64
msbuild.exe %BUILD_FILE% /t:clean /p:Configuration="%~1" /P:Platform=%2

IF ERRORLEVEL 1 (
  set BUILD_FAILED=1
)

msbuild.exe %BUILD_FILE% /t:sdv /p:inputs="/clean" /p:Configuration="%~1" /P:platform=%2

IF ERRORLEVEL 1 (
  set BUILD_FAILED=1
)

msbuild.exe %BUILD_FILE% /p:Configuration="%~1" /P:Platform=%2 /P:RunCodeAnalysisOnce=True

IF ERRORLEVEL 1 (
  set BUILD_FAILED=1
)

msbuild.exe %BUILD_FILE% /t:sdv /p:inputs="/check /devenv" /p:Configuration="%~1" /P:platform=%2

IF ERRORLEVEL 1 (
  set BUILD_FAILED=1
)

msbuild.exe %BUILD_FILE% /t:dvl /p:Configuration="%~1" /P:platform=%2

IF ERRORLEVEL 1 (
  set BUILD_FAILED=1
)

goto :eof

:split_target_tag
set TARGET=
set TAG=
for /f "tokens=1 delims=_" %%T in (%1) do (
    set TARGET=%%T
)
for /f "tokens=2 delims=_" %%T in (%1) do (
    set TAG=_%%T
)
goto :eof

:fail

exit /B 1
