@echo off
rem USAGE:
rem    RBUILD ffresh|fall /MD6 /MS6 /WD6 /WS6 /WD7 /WS7
rem	Flags are optional, defaults are:
rem     fall
rem     /md6

if '%_TEST%' == '1' @echo on

if "%1" == "/?" goto USAGE
if "%1" == "/h" goto USAGE
if "%1" == "/H" goto USAGE
if '%1' == '-?' goto USAGE
if '%1' == '-h' goto USAGE
if '%1' == '-H' goto USAGE
if '%1' == '?' goto USAGE

set _COMMUNICATE=\\Groundhog\communicate

if '%USERNAME%' == '' echo WHOA!  You must first set the environment variable USERNAME to your mailname!
if '%USERNAME%' == '' goto END

if exist %_COMMUNICATE%\*.bat echo First RBuild enlistment is busy - trying second...
if exist %_COMMUNICATE%\*.bat set _COMMUNICATE=\\Groundhog\communicate_2

if exist %_COMMUNICATE%\*.bat echo SORRY!  Both RBuild enlistments are busy...
if exist %_COMMUNICATE%\*.bat goto END

if exist rbuild.txt del rbuild.txt
set _CMDFILE=%_COMMUNICATE%\%USERNAME%.bat
set _RESFILE=%_COMMUNICATE%\%USERNAME%.log
set _DONFILE=%_COMMUNICATE%\%USERNAME%.don
if exist %_COMMUNICATE%\%USERNAME%.* del %_COMMUNICATE%\%USERNAME%.*

echo Sending command to Sync Project
echo Sending command to Sync Project>>rbuild.txt
echo CALL REMOTE1.BAT %USERNAME%  >~temp.bat

rem DEFAULT:
set _FLAGS=fall
set _TARGET=ppcmac\debug
if '%1' == '' goto SKIP

:LOOP
if '%1' == '' goto WAITFORRESULTS

if '%1' == 'fall' GOTO FALL
if '%1' == 'FALL' GOTO FALL
if '%1' == 'ffresh' GOTO FFRESH
if '%1' == 'FFRESH' GOTO FFRESH
if '%1' == 'fdepend' GOTO FFRESH
if '%1' == 'FDEPEND' GOTO FFRESH

set _TARGET=
if "%1" == "/WD6" set _TARGET=win\debug
if "%1" == "/wd6" set _TARGET=win\debug
if "%1" == "/WS6" set _TARGET=win\ship
if "%1" == "/ws6" set _TARGET=win\ship
if "%1" == "/MD6" set _TARGET=ppcmac\debug
if "%1" == "/md6" set _TARGET=ppcmac\debug
if "%1" == "/MS6" set _TARGET=ppcmac\ship
if "%1" == "/ms6" set _TARGET=ppcmac\ship
if "%1" == "/WD7" set _TARGET=win\debug.97
if "%1" == "/wd7" set _TARGET=win\debug.97
if "%1" == "/WS7" set _TARGET=win\ship.97
if "%1" == "/ws7" set _TARGET=win\ship.97

if '%_TARGET%' == '' ECHO Invalid Flag %1 ???
if '%_TARGET%' == '' GOTO END

:SKIP
echo Sending command to build %_TARGET% (%_FLAGS%)
echo Sending command to build %_TARGET% (%_FLAGS%) >>rbuild.txt
echo CALL REMOTE2.BAT %USERNAME% %_FLAGS% %_TARGET% >>~temp.bat
shift
goto LOOP

:FFRESH
set _FLAGS=ffresh
shift
if '%1' == '' goto SKIP
goto LOOP

:FALL
set _FLAGS=fall
shift
if '%1' == '' goto SKIP
goto LOOP

:WAITFORRESULTS
echo CALL REMOTE3.BAT %USERNAME% >>~temp.bat
echo Waiting for Results...
echo Waiting for Results... >>rbuild.txt
copy ~temp.bat %_CMDFILE% >nul
del ~temp.bat

:RESULTS
if exist %_RESFILE% goto RESULTSREAD
if exist %_DONFILE% goto END1
SLEEP 2
GOTO RESULTS

:RESULTSREAD
sleep 2
copy %_RESFILE% ~temp.txt >nul
del %_RESFILE%
type ~temp.txt >>RBUILD.TXT
type ~temp.txt
del  ~temp.txt
goto RESULTS

:END1
sleep 1
del %_DONFILE%
echo.
echo Results are logged in RBUILD.txt
echo.
GOTO END

:USAGE
echo USAGE:
echo RBUILD (fall/ffresh) (list of builds)
echo.
echo /wd6 = win\debug.96
echo /ws6 = win\ship.96
echo /md6 = ppcmac\debug.96
echo /ms6 = ppcmac\ship.96
echo /wd7 = win\debug.97
echo /ws7 = win\ship.97
echo.
echo Default is:
echo RBUILD fall /md6
echo.

:END
REM clear out environment variables
set _ssynclog=
set _FLAGS=
set _TARGET=
set _SSYNC=
set _CMDFILE=
set _DONFILE=
set _RESFILE=