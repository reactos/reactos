@echo off

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
set NO_BINPLACE=1

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

rem
rem Path trickyness (NT only): We want our final path to end up like this:
rem    path=<path set by ieenv.cmd>;<user's original path>;<mshtmenv additions>
rem
rem The problem is that ieenv.bat blows away your path entirely and you lose
rem   any additions you set in your global environment. As a result, we save
rem   the user's path, call ieenv.bat, and then reconstruct things ourselves
rem   afterward.
rem

rem
rem Put all interesting paths in %temp%\mshtmenv.tmp in reverse order.
rem   Duplicates are removed later.
rem
echo %_ntbindir%\private\inet\%mshtml%\perf\bin> %temp%\mshtmenv.tmp

rem Need to remove quotes from the existing path or it really messes us up.
echo %path:"=%>%temp%\mshtmenv2.tmp
for /F "tokens=1-26 delims=;=" %%a in (%temp%\mshtmenv2.tmp) do call :writepath "%%z" "%%y" "%%x" "%%w" "%%v" "%%u" "%%t" "%%s" "%%r" "%%q" "%%p" "%%o" "%%n" "%%m" "%%l" "%%k" "%%j" "%%i" "%%h" "%%g" "%%f" "%%e" "%%d" "%%c" "%%b"

call %_ntdrive%%_ntroot%\bin\ieenv.cmd

rem Have to put this here since it uses %_ntuser%
echo %_ntbindir%\private\developr\%_ntuser%>> %temp%\mshtmenv.tmp

echo %path:"=%>%temp%\mshtmenv2.tmp
for /F "tokens=1-26 delims=;=" %%a in (%temp%\mshtmenv2.tmp) do call :writepath "%%z" "%%y" "%%x" "%%w" "%%v" "%%u" "%%t" "%%s" "%%r" "%%q" "%%p" "%%o" "%%n" "%%m" "%%l" "%%k" "%%j" "%%i" "%%h" "%%g" "%%f" "%%e" "%%d" "%%c" "%%b"

path .

set /A linecnt=0
for /F "tokens=*" %%a in (%temp%\mshtmenv.tmp) do call :addtopath "%%a"

del /q %temp%\mshtmenv.tmp >nul 2>nul
del /q %temp%\mshtmenv2.tmp >nul 2>nul

:setenv
set ntdebug=ntsd
set ntdebugtype=both
set ntdbgfiles=1
set msc_optimization=/Odi
set BUILD_OPTIONS=%BUILD_OPTIONS% -w
set BUILD_DEFAULT=%BUILD_DEFAULT% %MSHTML%
set BUILD_PRODUCT=IE

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

:writepath
rem
rem Loop through all the passed arguments and write them out to the temp file
rem
if {%1} == {} goto :eof
if {%1} == {""} shift & goto :writepath
set newdir=%1
echo %newdir:"=%>> %temp%\mshtmenv.tmp
shift
goto :writepath

:addtopath
rem
rem See if the given argument (a directory) occurs later in our file containing
rem   directories to add to our path. If not, then add it to our path.
rem
set /A linecnt=%linecnt%+1
set newdir=%1
set newdir=%newdir:"=%
for /F "skip=%linecnt% tokens=*" %%i in (%temp%\mshtmenv.tmp) do if /I {"%%i"} == {%1} goto :eof
path %newdir%;%path%
goto :eof

:done
