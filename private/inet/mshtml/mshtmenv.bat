@echo off

if "%OS%"=="Windows_NT" goto WinNT

echo.
echo This batch file does not support non-NT systems.
echo.
goto done

:WinNT

rem
rem set the MSHTML environment variable to the name of the mshtml directory
rem  (:getmshtml is at the bottom of this file).
rem
call :getmshtml %~p0
if "%MSHTML%" == "" set MSHTML=mshtml

rem We can't call shift until we're done using %0!

set __ia64=

:nextarg

if "%1"=="-ia64" goto :ia64
if "%1"=="-axp64" goto :axp64
goto :endargs

:ia64
set __ia64=1
shift
goto :nextarg

:axp64
set __axp64=1
shift
goto :nextarg

:endargs

if not "%1"=="" set _ntdrive=%1
if not "%2"=="" set _ntroot=%2

rem
rem Extract _ntdrive and _ntroot from the path of this batch file.
rem   _ntroot is expected to be exactly three characters long (e.g. \nt, \ie)
rem
if "%_ntdrive%"=="" set _ntdrive=%~d0
if NOT "%_ntroot%"=="" goto skip
set _ntroot=%~f0
set _ntroot=%_ntroot:~2,3%

:skip
set _ntbindir=%_ntdrive%%_ntroot%
set _ieroot=%_ntroot%
set use_switches_in_retail_build=1
if not "%processor_architecture%" == "x86" set build_default_targets=-%processor_architecture%

set mshtmenv=1
set no_binplace=1

rem
rem Path trickyness (NT only): We want our final path to end up like this:
rem    path=<path1st>;<path set by ntenv.cmd>;<user's original path>;<mshtmenv additions>
rem
rem The problem is that duplicates end up in the list because ntenv.cmd
rem   requires some of the things it puts in to already be in the path. As a
rem   result, we save the user's path, substitute in a temporary path before
rem   calling ntenv.bat, and the pull out everything it added afterward. Then
rem   we have the luxury of constructing the path exactly as we want it.
rem
set userspath=%path%
set path=%path%;%_ntbindir%\public\tools;c:\pathmarker

call %_ntdrive%%_ntroot%\bin\ieenv.cmd

set ntenvpath=%path:*c:\pathmarker;=%
if "%ntenvpath%" == "" set ntenvpath=%path%
set path=%_ntbindir%\idw\path1st;%ntenvpath%;%userspath%;%_ntbindir%\bin\%processor_architecture%;%_ntbindir%\private\inet\%mshtml%\perf\bin;%_ntbindir%\private\developr\%_ntuser%
set ntenvpath=
set userspath=

:setenv
set ntdebug=ntsd
set ntdebugtype=both
set ntdbgfiles=1
set msc_optimization=/Odi
set BUILD_OPTIONS=%BUILD_OPTIONS% -w
set BUILD_DEFAULT=%BUILD_DEFAULT% %MSHTML%
set BUILD_PRODUCT=IE

if not "%__ia64%"=="1" goto :noia64env

set host_tools="PATH=%PATH%"
set build_default_targets=-ia64
set host_targetcpu=i386
set host_target_directory=ia64
set host_target_defines=ia64=1 i386=0 genia64=1
set ntia64default=1
set ia64=1
set ia64_warning_level=-W3 -D_M_IA64
set ntdebug=ntsd
set ntdebugtype=windbg
set build_options=%build_options% -ia64
set iebuild_options=%iebuild_options% ~iextag ~imgfilt
path=%_NTDRIVE%%_NTROOT%\mstools\win64;%path%

:noia64env

if not "%__axp64%"=="1" goto :noaxp64env

set host_tools="PATH=%PATH%"
set build_default_targets=-axp64
set host_targetcpu=alpha
set host_target_directory=axp64
set host_target_defines=axp64=1 alpha=0 genaxp64=1
set ntaxp64default=1
set ntalphadefault=
set axp64=1
set alpha=
set axp64_warning_level=-W3 -D_M_AXP64
set ntdebug=ntsd
set ntdebugtype=windbg
set build_options=%build_options% -axp64
set iebuild_options=%iebuild_options% ~iextag
path=%_NTDRIVE%%_NTROOT%\mstools\win64;%path%

:noaxp64env

if "%use_mshtml_incremental_linking%"=="" set use_mshtml_incremental_linking=%3
if %NUMBER_OF_PROCESSORS% GTR 1 if "%1" == "" set use_mshtml_incremental_linking=0
if "%use_mshtml_incremental_linking%"=="" set use_mshtml_incremental_linking=1
if "%use_mshtml_incremental_linking%"=="1" set use_mshtml_pdb_rules=1

rem 
rem  We no longer need to update the i386mk.inc or alphamk.inc files.  But we need to make sure
rem  they are not checked out and that you have the latest version.
rem

copy %_ntdrive%%_ntroot%\public\oak\bin\i386mk.inc %temp%\i386mk.inc 1>nul: 2>nul:
if errorlevel 1 goto nocopy1
copy %temp%\i386mk.inc %_ntdrive%%_ntroot%\public\oak\bin\i386mk.inc 1>nul: 2>nul:
if errorlevel 1 goto doalpha
echo mshtmenv: i386mk.inc is read/write; performing "in -i & ssync"
pushd %_ntdrive%%_ntroot%\public\oak\bin\
in -!fi i386mk.inc
ssync i386mk.inc
popd
goto :doalpha
:nocopy1
echo mshtmenv: Unable to copy i386mk.inc to %temp%\i386mk.inc
goto :doalpha

:doalpha
copy %_ntdrive%%_ntroot%\public\oak\bin\alphamk.inc %temp%\alphamk.inc 1>nul: 2>nul:
if errorlevel 1 goto nocopy2
copy %temp%\alphamk.inc %_ntdrive%%_ntroot%\public\oak\bin\alphamk.inc 1>nul: 2>nul:
if errorlevel 1 goto alphadone
echo mshtmenv: alphamk.inc is read/write; performing "in -i & ssync"
pushd %_ntdrive%%_ntroot%\public\oak\bin\
in -!fi alphamk.inc
ssync alphamk.inc
popd
goto :alphadone
:nocopy2
echo mshtmenv: Unable to copy alphamk.inc to %temp%\alphamk.inc
goto :alphadone

:alphadone
goto :done

:getmshtml
rem
rem Strip off one directory name at a time until we get to the last directory
rem  name. (e.g. d:\nt\private\inet\mshtml\ == mshtml) There's no way to
rem  strip off from the end, we have to get rid of one directory at a time
rem  from the beginning.
rem
set temppath=%1
set temppath=%temppath:*\=%
if NOT "%temppath%" == "" call :getmshtml %temppath% & goto :eof
set MSHTML=%1
rem Get rid of trailing slash
set MSHTML=%MSHTML:\=%
goto :eof

:done

set __ia64=
set __axp64=
