@echo off
rem USAGE:
rem    drop dir platform flavor (destdir)
rem Ex:
rem    drop win\debug win debug (\\Trigger\Trident)

if '%_TEST%' == '1' @echo on
setlocal
pushd %_F3DIR%

rem If 3 required parameters aren't entered, print out syntax/usage help
if '%3' == '' GOTO Usage

if '%_DROP% == '0' goto END

rem ****STANDARDIZE CAPITALIZATION*************************
rem Note: buildall.pl demonstrates proper capitalization of these parameters.
set _PLATFORM=%2
if '%_PLATFORM%' == 'WIN' set _PLATFORM=x86
if '%_PLATFORM%' == 'Win' set _PLATFORM=x86
if '%_PLATFORM%' == 'win' set _PLATFORM=x86
if '%_PLATFORM%' == 'X86' set _PLATFORM=x86

if '%_PLATFORM%' == 'PPCMAC' set _PLATFORM=ppcmac
if '%_PLATFORM%' == 'Ppcmac' set _PLATFORM=ppcmac

set _FLAVOR=%3
if '%_FLAVOR%' == 'SHIP' set _FLAVOR=ship
if '%_FLAVOR%' == 'Ship' set _FLAVOR=ship
if '%_FLAVOR%' == 'DEBUG' set _FLAVOR=debug
if '%_FLAVOR%' == 'Debug' set _FLAVOR=debug

set _SOURCEDIR=build\%1\bin

REM Special consideration when not part of automated build\drop:
if '%_DROPNAME%' == '' set _DROPNAME=special

rem Use today's date as a build date for a non-automated Trident drop

if NOT '%_DESTDIR%' == '' goto GOTDESTDIR
%_F3QADIR%\TOOLS\%PROCESSOR_ARCHITECTURE%\GETBLDDA.EXE %_DROPNAME%
call ~v.bat
del /f /q ~v.bat

:GOTDESTDIR

if '%_DESTDIR%' == '' set _DESTDIR=%4
if '%_DESTDIR%' == '' set _DESTDIR=\\TRIGGER\TRIDENT\SPECIAL

rem XCOPY flags used in copying files from source to destination.
rem Current flags:
rem /F=Display fully qualified filenames
rem /I=Assume destination is a directory (not a file)
rem /V=Verify copy 
set _XCOPYFLAGS=/f /i /v

rem Utility program locations
set _NOW=%!NOW%
if '%_NOW%' == '' set _NOW=%_F3QADIR%\tools\%PROCESSOR_ARCHITECTURE%\now.exe
set _SPLITSYM=%_F3DIR%\tools\%PROCESSOR_ARCHITECTURE%\bin\splitsym.exe
if NOT '%_FLAVOR%' == 'ship' set _SPLITSYM=REM 
set _MAPSYM=%_F3DIR%\tools\%PROCESSOR_ARCHITECTURE%\bin\mapsym.exe
set _IEXPRESS=%_F3QADIR%\tools\%PROCESSOR_ARCHITECTURE%\iexpress.exe

rem *******************************************************
echo Dropping to %_DESTDIR%...
%_NOW% ---------------------------- Starting %1 drop

rem *******************************************************
Echo Making Dirs
if not exist %_DESTDIR% md %_DESTDIR%
if not exist %_DESTDIR%\%_PLATFORM% md %_DESTDIR%\%_PLATFORM%
if not exist %_DESTDIR%\%_PLATFORM%\%_FLAVOR% md %_DESTDIR%\%_PLATFORM%\%_FLAVOR%
if not exist %_DESTDIR%\%_PLATFORM%\%_FLAVOR%\LOG md %_DESTDIR%\%_PLATFORM%\%_FLAVOR%\LOG
if not exist %_DESTDIR%\%_PLATFORM%\%_FLAVOR%\BIN md %_DESTDIR%\%_PLATFORM%\%_FLAVOR%\BIN
set _COPYDESTDIR=%_DESTDIR%\%_PLATFORM%\%_FLAVOR%

rem *******************************************************
echo Deleting IDB files
del /s /q %_SOURCEDIR%\*.idb

rem *******************************************************
echo Copying make.bat
XCOPY build\%1\make.bat  %_DESTDIR%\%_PLATFORM%\%_FLAVOR%\log %_XCOPYFLAGS%

:96PSETUP
rem *******************************************************
echo Dropping Trident Setup...

pushd %_SOURCEDIR%
%_SPLITSYM% -a mshtml.dll
%_SPLITSYM% -a mshtmenu.dll
%_SPLITSYM% -a mshtmpad.exe

%_MAPSYM% mshtml.MAP
%_MAPSYM% mshtmenu.MAP
%_MAPSYM% mshtmpad.MAP
%_MAPSYM% mshtmdbg.MAP
popd

@echo on
rem Create diskette images disk1,disk2,..,diskn underneath a temp directory
if not exist %TEMP%\%_FLAVOR% md %TEMP%\%_FLAVOR%
pushd %TEMP%\%_FLAVOR%
%_F3DIR%\src\f3\acme\trident\diamond /F %_F3DIR%\src\f3\acme\trident\%_FLAVOR%.96P\tri%_FLAVOR%.ddf /D SLM_ROOT=%_F3DIR% /D BLDDATE=%_BLDDATE% 2>&1 | tee setup.log
find /i "ERROR" setup.log >setup.err

if errorlevel 1 goto NOSETUPPROBLEMS
echo ******************************************>>setup.err
type %TEMP%\%_FLAVOR%\setup.rpt >>setup.err

if "%_SENDFAIL%" == "" goto NOSETUPPROBLEMS
if "%!MAILPATH%" == "" goto NOSETUPPROBLEMS
echo /s Trident Setup is missing files! %_SENDFAIL% >>setup.err
copy setup.err %!MAILPATH%

:NOSETUPPROBLEMS
del /q/f %TEMP%\%_FLAVOR%\setup.rpt

rem Move our created "setup.inf" file to disk1
XCOPY %TEMP%\%_FLAVOR%\trident.inf %TEMP%\%_FLAVOR%\disk1 %_XCOPYFLAGS%
del %TEMP%\%_FLAVOR%\trident.inf

REM Build version and flavor go into STF file.
rename %TEMP%\%_FLAVOR%\disk1\TRIDENT.STF TEMP.STF
ECHO App Name	Microsoft Trident %_FLAVOR% build>%TEMP%\%_FLAVOR%\disk1\TRIDENT.STF
ECHO App Version	%_FLAVOR%.%_DROPNAME% >>%TEMP%\%_FLAVOR%\disk1\TRIDENT.STF
TYPE %TEMP%\%_FLAVOR%\disk1\TEMP.STF >>%TEMP%\%_FLAVOR%\disk1\TRIDENT.STF
DEL %TEMP%\%_FLAVOR%\disk1\TEMP.STF

rem Move this tree of diskette images to our destination directory
XCOPY %TEMP%\%_FLAVOR% %_COPYDESTDIR% /e /f /i /v

del /q/f %_COPYDESTDIR%\setup.err
del /q/f %_COPYDESTDIR%\setup.log

rem Clean up and remove temp directory
popd
del /s/q/f %TEMP%\%_FLAVOR%
rd  /s/q   %TEMP%\%_FLAVOR%

GOTO SKIPSETUP
rem *******************************************************
rem IEXPRESS!
if not exist c:\setup md c:\setup
xcopy %_f3qadir%\setup\*.* 					%_COPYDESTDIR%\bin %_XCOPYFLAGS%
xcopy %_f3qadir%\setup\%_FLAVOR%\*.* 				%_COPYDESTDIR%\bin %_XCOPYFLAGS%

REM more copy statements go here

pushd c:\setup
%_IEXPRESS% /n /q minexe.cdf
call sign.bat minimum.exe
xcopy Minimum.exe %_COPYDESTDIR% %_XCOPYFLAGS%
xcopy c:\setup\*.* %_COPYDESTDIR%\bin %_XCOPYFLAGS%
del /s/q/f c:\setup
rd  /s/q   c:\setup

:SKIPSETUP
xcopy %_f3dir%\tools\%PROCESSOR_ARCHITECTURE%\bin\misc\*.* 	%_COPYDESTDIR%\bin %_XCOPYFLAGS%
xcopy %_f3dir%\%_SOURCEDIR%\*.* 				%_COPYDESTDIR%\bin %_XCOPYFLAGS%
xcopy %_f3dir%\src\f3\acme\trident\redist\*.htm 		%_COPYDESTDIR%\bin %_XCOPYFLAGS%
xcopy %_f3dir%\build\%1\SDK\*.* 				%_COPYDESTDIR%\bin %_XCOPYFLAGS%
xcopy %_F3DIR%\src\f3\drt\activex\%_flavor%\*.* 		%_COPYDESTDIR%\bin %_XCOPYFLAGS%

copy %_f3dir%\build\%1\f3types\mshtml.tlb 			%_COPYDESTDIR%\bin 
copy %_F3DIR%\src\site\include\mshtmcid.h 			%_COPYDESTDIR%\bin 
copy %_F3DIR%\src\site\include\mshtmhst.h 			%_COPYDESTDIR%\bin 


%_NOW% ---------------------------- Finished %1 drop
GOTO END

:USAGE
rem *******************************************************
echo.
echo USAGE:
echo    drop dir platform flavor (destdir)
echo where,
echo    dir       = directory in which to run make.bat (e.g., win\debug)
echo    platform  = x86
echo    flavor    = debug or ship or profile
echo    destdir   = destination directory to drop into (e.g., \\TRIGGER\Trident\Bld9999)
echo.

:END
popd
endlocal
