@echo off
rem USAGE:
rem    do not call BUILD.BAT directly
rem
rem Assumptions:
rem	_F3QADIR  and _F3DIR are set
rem     the f3qa\tools\x86 dir is in the path
rem     SLM is on the path
rem     the network is running
rem     the machine is running NT
rem	SMailSvr is running

if '%_TEST%' == '1' @echo on
set STAY=/C
rem When launching with CMD.EXE, /K makes windows stay up for testing
if '%_TEST%' == '1' set STAY=/K

if '%PROCESSOR_ARCHITECTURE%' == '' echo The build will only run on NT!
if '%PROCESSOR_ARCHITECTURE%' == '' goto ABORTEND

rem Make sure flags have been set in a template file (i.e. NIGHTLY.BAT)
if '%_TEMPLATE%' == '' echo DO NOT call build.bat directly!&goto ABORTEND

if '%_F3DIR%' == '' set _F3DIR=D:\FORMS3
if '%_F3QADIR%' == '' set _F3QADIR=%_F3dir%\build\scripts

if not exist %_F3DIR%\log md %_F3DIR%\log
pushd %_F3DIR%

set !BUILDDONE=%_F3DIR%\LOG\BUILDDON
rem If there are any builds running, there will be BUILDDON.* files in the log directory...
if '%_ALLOWMULTI%' == '0' if exist %!BUILDDONE%.* ECHO There is already a build in process!& GOTO ABORTEND
if exist %!BUILDDONE%.* del /f /q %!BUILDDONE%.*
if exist %_F3DIR%\log\regver.run del %_F3DIR%\log\regver.run

set !SSYNCLOG=%_F3DIR%\LOG\SSYNC.LOG
set !COPYLOG=%_F3DIR%\LOG\COPY.LOG
set !BLDRES=%_F3DIR%\LOG\BLDRES.LOG

if exist %!SSYNCLOG% del /f /q %!SSYNCLOG%
if exist %!COPYLOG% del /f /q %!COPYLOG%
if exist %!BLDRES% del /f /q %!BLDRES%

if '%PROCESSOR_ARCHITECTURE%' == 'x86' set !F3TOOLSDIR=%_F3QADIR%\tools\x86
if '%PROCESSOR_ARCHITECTURE%' == 'ALPHA' set !F3TOOLSDIR=%_F3QADIR%\tools\alpha
set !F3BUILDDIR=%_F3QADIR%\build

set !TRIGGERDEST=\\F3QA\TP3\Triggers
set !MAILPATH=\\trigger\mailout\pending
if '%_MAIL%' == '0' set !MAILPATH=nul
if '%_TP3%' == '0' set !TRIGGERDEST=nul

set !NOW=%!F3TOOLSDIR%\now.exe
set !SLEEP=%!F3TOOLSDIR%\sleep.exe
set !CHOICE=%!F3TOOLSDIR%\Choice
set !PERL=%!F3TOOLSDIR%\perl.exe
set !DSKSPACE=%!F3TOOLSDIR%\dskspace.exe
SET !ROBOCOPY=%!F3TOOLSDIR%\robocopy.exe
if NOT '%PROCESSOR_ARCHITECTURE%' == 'x86' set !DSKSPACE=REM 
REM This ought to work, but it doesn't:
REM set !AWK=%!F3TOOLSDIR%\awk.exe
set !AWK=awk.exe
set !BUILDDB=REM 
set _ComputerName=%COMPUTERNAME%
set !TESTRESULTS=\\triatcm\www

rem  Since we might be on a scheduled system process, we must declare all
rem  network connections here for them to all connect properly:
net use /persistent:no
net use \\trango\trident /delete
net use \\trango\trident /user:REDMOND\frm3bld _frm3bld
net use \\TRIATCM\TP3 /delete
net use \\TRIATCM\TP3 /user:REDMOND\formsqa _formsqa
net use \\trigger\trident /delete
net use \\trigger\trident /user:REDMOND\frm3bld _frm3bld
net use \\trigger\mailout /delete
net use \\trigger\mailout /user:REDMOND\frm3bld _frm3bld
net use \\stork\formslm /delete
net use \\stork\formslm /user:REDMOND\formsqa _formsqa
net use \\FormBuild\Build$ /delete
net use \\FormBuild\Build$ /user:REDMOND\frm3bld _frm3bld
net use \\triatcm\www /delete
net use \\triatcm\www /user:REDMOND\frm3bld _frm3bld

rem ******CHECK DISK SPACE************************************************
REM temporarily skipping due to problems

if %_DROP% == '0' GOTO SSYNC
%!dskspace% %_DESTROOT%\ %!ProjSize%
if not errorlevel 1 GOTO SSYNC
%!Choice% /t:Y,10 DANGER!  Disk space inadequate, switch to local drive and continue
if errorlevel 2 GOTO ABORTEND
rem Step through local drives C,D,E,F to try to find room:
set _DESTROOT=c:
%!dskspace% %_DESTROOT%\ %!ProjSize%
if not errorlevel 1 GOTO NEWDEST
set _DESTROOT=d:
%!dskspace% %_DESTROOT%\ %!ProjSize%
if not errorlevel 1 GOTO NEWDEST
set _DESTROOT=e:
%!dskspace% %_DESTROOT%\ %!ProjSize%
if not errorlevel 1 GOTO NEWDEST
set _DESTROOT=f:
%!dskspace% %_DESTROOT%\ %!ProjSize%
if not errorlevel 1 GOTO NEWDEST
echo WARNING!  Drop area was full, and no local space is available>%!MAILPATH%\disk.ful
echo /S BUILD Destination disk full /T BENCHR /T a-willr >>%!MAILPATH%\disk.ful
goto ABORTEND
:NEWDEST
echo>>%!BLDRES%
echo WARNING!  Drop area was full, project dropped to %_DESTROOT%>>%!BLDRES%
echo>>%!BLDRES%

:SSYNC
rem ******PERFORM FORMS3 PROJECT SSYNC***********************************
rem  If slm.ini is missing, it must be frozen source-
if NOT EXIST %_F3DIR%\slm.ini goto SKIPSSYNC
if NOT '%_SSYNC%' == '1' goto SKIPSSYNC
copy %!TESTRESULTS%\pre.htm %!ssynclog%
%!NOW% ------------ Starting ssync >>%!ssynclog%
set ctr=

:DOSSYNC
rem Use a string of xxx's to keep track of minutes ticking.
set ctr=x%ctr%

if not '%_OVERRIDE%' == '1'  goto NOTOVERRIDE
echo Overriding project lock...
ssync -! -faq -l %!ssynclog%
goto MAKESLMLOG

:NOTOVERRIDE
cookie -r -c "ssyncing build machine" 2>>%!ssynclog% || goto SSYNCERR
ssync -faq -l %!ssynclog%
cookie -f 2>>%!ssynclog%

:MAKESLMLOG
echo FILE UPDATES IN THE PAST 24 HOURS...>>%!ssynclog%
log -i -a -z -f -t .-1 >>%!ssynclog% 2>nul
%!NOW% ------------ Finished ssync >>%!ssynclog%

rem Ensure that we're building with the correct oleaut32.dll.  mktyplib.exe
rem uses this file.  This assumes that oleaut32.dll has been removed from the
rem KnownDLLs registry section, and that it's not in use.
rem copy %_F3DIR%\tools\oleaut\%PROCESSOR_ARCHITECTURE%\oleaut32.* %SystemRoot%\system32 >>%!ssynclog%

goto GETVER

:SSYNCERR
cookie
if '%ctr%' == 'xxxxxxxxxxxxxxx' GOTO GIVEUP
if '%ctr%' == 'x' GOTO COOKIEWARN

echo ERROR: Couldn't get read cookie.  Sleeping for 2 minutes... >>%!ssynclog%
@echo ERROR: Couldn't get read cookie.  Sleeping for 2 minutes...
%!SLEEP% 120
echo Retrying... >>%!ssynclog%
goto DOSSYNC

:GIVEUP
echo WARNING!  Project overrode a project lock!>>%!BLDRES%
echo. >>%!BLDRES%
rem if '%_UPDATEVER%' == '1' %!perl% %!F3BUILDDIR%\setver.pl >>%!ssynclog%
ssync -! -faq -l %!ssynclog%
echo FILE UPDATES IN THE PAST 24 HOURS...>>%!ssynclog%
log -i -a -z -f -t .-1 >>%!ssynclog% 2>nul
%!NOW% ------------ Finished ssync >>%!ssynclog%
goto GETVER

:COOKIEWARN
rem Send mail to developers warning of impending cookie override
echo WARNING!  Build machine will override the following project lock in 30 minutes: >~T.ZZZ
echo. >>~T.ZZZ
cookie 2>>~T.ZZZ
echo /s !Forms3 project is locked! %_SENDFAIL% >>~T.ZZZ
copy ~T.ZZZ %!MAILPATH%\%_DROPNAME%.NOC
del /F/Q ~T.ZZZ
echo Sent mail about read cookie.  Sleeping for 2 minutes... >>%!ssynclog%
@echo Sent mail about read cookie.  Sleeping for 2 minutes...
%!SLEEP% 120
echo Retrying... >>%!ssynclog%
goto DOSSYNC

:SKIPSSYNC
%!now% -------------- Not ssync-ing build >>%!ssynclog%

:GETVER
rem ******GET VERSION NUMBER*****************************************
if '%_UPDATEVER%' == '1' %!perl% %!F3BUILDDIR%\setver.pl >>%!ssynclog%

REM if it is '2' then increment minor version only
if '%_UPDATEVER%' == '2' sadmin setpv ..+1 >>%!ssynclog%

if '%_DROP%' == '0' GOTO SKIPSETUP
if not '%_DROPNAME%' == '' goto GOTNAME

%!PERL% %!F3BUILDDIR%\GETVER.PL
call ~v.bat
del /f /q ~v.bat
if '%_DROPNAME%' == '' set _DROPNAME=SPECIAL
if exist %_DESTROOT%\%_DROPNAME%\. set _DROPNAME=%_DROPNAME%.new

:GOTNAME
rem Construct a build date _BLDDATE (yyyy-mm-dd) from the drop name _DROPNAME
%!F3TOOLSDIR%\GETBLDDA.EXE %_DROPNAME%
call ~v.bat
del /f /q ~v.bat

set _DESTDIR=%_DESTROOT%\%_DROPNAME%
md %_DESTDIR% >nul
echo ***
echo Building %_DROPNAME%, which will be dropped into %_DESTROOT%
echo ***
attrib +h %_DESTDIR%

:SKIPSETUP
title Building %_DESTROOT%\%_DROPNAME%
echo %_DROPNAME% Build Results:>%!BLDRES%
echo.>>%!BLDRES%
echo DROP		COMPLETED	PROBLEMS FOUND>>%!BLDRES%
rem Save all environment variables beginning with '_'
set | %!AWK% "/^_+/ {print}" >%_F3DIR%\LOG\SET.LOG
rem Save logs, and update database on F3QA
%!ROBOCOPY% %_F3DIR%\log %_DESTDIR%\archive\log /s

REM **************Update testing web page****************************
type %!TESTRESULTS%\t1.txt >>%!TESTRESULTS%\number.htm
echo %_DROPNAME%>>%!TESTRESULTS%\number.htm
type %!TESTRESULTS%\t2.txt >>%!TESTRESULTS%\number.htm
echo %_DROPNAME%>>%!TESTRESULTS%\number.htm
type %!TESTRESULTS%\t3.txt >>%!TESTRESULTS%\number.htm
echo.>>%!TESTRESULTS%\number.htm

md %!TESTRESULTS%\%_DROPNAME%
copy %!TESTRESULTS%\template\*.* %!TESTRESULTS%\%_DROPNAME%
copy %!ssynclog% %!TESTRESULTS%\%_DROPNAME%\ssync.htm
copy %!TESTRESULTS%\pre.htm %!TESTRESULTS%\%_DROPNAME%\build.htm

:SKIPWEB
rem *************WRITE OUT THE FILE THAT LAUNCHES ALPHA BUILD***********
if '%A96%' == '' goto SkipAlpha
if '%A96%' == '0' goto SkipAlpha
set !GoAlpha=\\FormBuild\Build$\GoAlpha.bat
del /f /q %!GoAlpha%
echo>>%!GoAlpha% set _DROPNAME=%_DROPNAME%
echo>>%!GoAlpha% set _DESTDIR=%_DESTDIR%
echo>>%!GoAlpha% set _RELEASE_CANDIDATE=%_RELEASE_CANDIDATE%
echo>>%!GoAlpha% set _MAKEFLAGS=%_MAKEFLAGS%
echo>>%!GoAlpha% set _CLEANMAKE=%_CLEANMAKE%
echo>>%!GoAlpha% set _MAIL=%_MAIL%
echo>>%!GoAlpha% set _SENDFAIL=%_SENDFAIL%
echo>>%!GoAlpha% set _DROP=%_DROP%
echo>>%!GoAlpha% set _TP3=%_TP3%
echo>>%!GoAlpha% set _RELEASE=%_RELEASE%

REM this will cause problem d:\forms96 <> \\formbuild\forms96$
REM echo>>%!GoAlpha% set _F3DIR=%_F3DIR%

:SKIPALPHA
rem ******SPAWN THE REQUIRED MAKES*********************************
%!perl% %!F3BUILDDIR%\BuildAll.PL %_MAXRUN%

rem *************SET UP FILES FOR SENDING RESULT MAIL****************
echo.>>%!BLDRES%
echo Drop point:	%_DESTDIR%>>%!BLDRES%
echo.>>%!BLDRES%

echo To install Trident (debug):>>%!BLDRES%
REM echo 	Minimum:	file:%_DESTDIR%\x86\debug\minimum.exe>>%!BLDRES%
echo 		file:%_DESTDIR%\x86\debug\disk1\setup.exe>>%!BLDRES%
echo. >>%!BLDRES%
echo To install Trident (ship):>>%!BLDRES%
REM echo 	Minimum:	file:%_DESTDIR%\x86\ship\minimum.exe>>%!BLDRES%
echo 		file:%_DESTDIR%\x86\ship\disk1\setup.exe>>%!BLDRES%

:NODROPTS
echo.>>%!BLDRES%
copy %!BLDRES% %_DESTDIR%

copy %!TESTRESULTS%\pre.htm %!TESTRESULTS%\%_DROPNAME%\build.htm
type %!BLDRES% >>%!TESTRESULTS%\%_DROPNAME%\build.htm

echo %_SENDPASS% /S %_DROPNAME% Trident Build Results>>%!BLDRES%
copy %!BLDRES% %!MAILPATH%\%_DROPNAME%.RES

if '%_DROP%' == '0' GOTO END
if '%_DROPSRCTOOLS%' == '0' GOTO END
rem ******DROP SOURCE AND TOOLS************************************
rem COPY THE SOURCE CODE AND TOOL DIRS, USING THE /M SWITCH
rem (copies files with the archive bit set, then turns it off).
echo Dropping Source and Tools...
%!ROBOCOPY% %_F3DIR%\src %_DESTDIR%\archive\src /s
%!ROBOCOPY% %_F3DIR%\help %_DESTDIR%\archive\help /s
%!ROBOCOPY% %_F3DIR%\tools %_DESTDIR%\archive\tools /s
%!ROBOCOPY% %_F3DIR%\log %_DESTDIR%\archive\log /s
xcopy %_F3DIR%\version.h %_DESTDIR%\archive /i/c/r/d/v >nul
xcopy %_F3DIR%\*.bat %_DESTDIR%\archive\log /i/c/r/d/v >nul
attrib -h %_DESTDIR%

:SKIPHELPDROP
rem *************WRITE OUT THE FILE THAT SETS THE ENV VAR***********
echo set _CURRENT_F3_VER=%_DROPNAME%>%_DESTROOT%\curf3ver.bat

:END
rem Don't leave the network connections open...
net use \\trango\trident /delete >nul
net use \\F3QA\TP3 /delete  >nul
net use \\trigger\f3drop /delete  >nul
net use \\trigger\trident /delete >nul
net use \\trigger\mailout /delete >nul
net use \\brant\ddtdrop /delete  >nul
net use \\basic3\vbarel /delete >nul
net use \\stork\formslm /delete >nul
net use \\FormBuild\Build$ /delete >nul
net use \\F3qa\results /delete >nul

title Done
type %!BLDRES%

:ABORTEND
popd
endlocal