@echo off

if (%1) == (bld) goto BLD

set CMDNAME=%_NTBINDIR%\private\shell\iert\%0

if (%1) == (?) goto USAGE

:GETNEXTENV
if (%1) == () goto ENVDONE
if (%1) == (iedev) set IEDEV=1
if (%1) == (iedev) shift
if (%1) == (clean) set CLEAN=-cC
if (%1) == (clean) set WCLEAN=clean2
if (%1) == (clean) shift
set PARAMS=%PARAMS% %1
shift
goto GETNEXTENV

:ENVDONE

del %_NTBINDIR%\private\shell\iert\build.dir 1>nul: 2>nul:

if (%IEDEV%) == () goto START
call %CMDNAME% bld %_NTBINDIR%\private\iedev call iebuild %CLEAN% %PARAMS%
call %CMDNAME% bld %_NTBINDIR%\private\windows nmake -f makefil0 %WCLEAN% %PARAMS%

:START
call %CMDNAME% bld %_NTBINDIR%\private\shell\iert call iebuild %CLEAN% %PARAMS%
call %CMDNAME% bld %_NTBINDIR%\private\shell\lib call iebuild %CLEAN% %PARAMS%
call %CMDNAME% bld %_NTBINDIR%\private\shell\shell32 call iebuild %CLEAN% %PARAMS%
call %CMDNAME% bld %_NTBINDIR%\private\shell\cpls\desknt call iebuild %CLEAN% %PARAMS%
call %CMDNAME% bld %_NTBINDIR%\private\shell\cpls\deskw95 call iebuild %CLEAN% %PARAMS%
call %CMDNAME% bld %_NTBINDIR%\private\inet\wininet call iebuild %CLEAN% %PARAMS%
call %CMDNAME% bld %_NTBINDIR%\private\inet\urlmon\idl call iebuild %CLEAN% %PARAMS%
call %CMDNAME% bld %_NTBINDIR%\private\inet\urlmon call iebuild %CLEAN% %PARAMS%
call %CMDNAME% bld %_NTBINDIR%\private\inet\xml call iebuild %CLEAN% %PARAMS%
call %CMDNAME% bld %_NTBINDIR%\private\inet\mshtml\imgfilt call iebuild %CLEAN% %PARAMS%
cd %_NTBINDIR%\private\shell\iert
goto DONE

:BLD
echo *********************************************************
echo *** %2
echo *********************************************************
cd %2
echo ********************************************************* >> %_NTBINDIR%\private\shell\iert\build.dir
echo %3 %4 %5 %6 %7 %8 %9 >> %_NTBINDIR%\private\shell\iert\build.dir
%3 %4 %5 %6 %7 %8 %9
dir build* >> %_NTBINDIR%\private\shell\iert\build.dir
goto DONE

:USAGE
echo %0 iedev clean anyiebuildcmd
goto DONE

:DONE
set PARAMS=
set IEDEV=
set CLEAN=
set WCLEAN=
set CMDNAME=


