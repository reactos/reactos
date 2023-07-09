@echo off
rem USAGE:
rem    bldcore dir platform flavor flag
rem where,
rem  1  dir       = directory in which to run make.bat
rem  2  platform  = x86 or alpha
rem  3  flavor    = debug or ship or profile
rem  4  flag      = extension for mail etc.

setlocal
IF '%_TEST%' == '1' @echo on

title Building %1
pushd %_F3DIR%\build
set _flag=%4

REM ************Set local options for this make*******************
set p=%%
set use=_MAKEFLAGS _CLEANMAKE _MAIL _RELEASE _TP3 _SENDPASS _SENDFAIL _DROP
set use=%use% _NOF3TEXT _USERTLFLAGS
for %%f in (%use%) do echo if not "%p%%_FLAG%%%f%p%" == "" set %%f=%p%%_FLAG%%%f%p%>~tt.BAT&call ~tt.bat
del /f /q ~tt.bat

if exist %_F3DIR%\build\%1\log\*.LOG del /f /q %_F3DIR%\build\%1\log\*.LOG
set !BUILDLOG=%_F3DIR%\build\%1\log\BUILD.LOG
set !ERRORLOG=%_F3DIR%\build\%1\log\ERROR.LOG
set !SETLOG=%_F3DIR%\build\%1\log\SET.LOG
set !DROPLOG=%_F3DIR%\build\%1\log\DROP.LOG

REM ********DON'T LET THE TEMP DIRECTORIES COLLIDE******************
set tmp=%tmp%\%_flag%
if not exist %tmp% md %tmp%
set temp=%tmp%

REM ********WE'LL DELETE THIS FILE WHEN BUILD COMPLETE**************
%!NOW% >%!BUILDDONE%.%_FLAG%

rem ********CLEANMAKE? -DELETE TREE ********************************
if "%_MAKEFLAGS%" == "fall" goto SKIPCLEAN
if "%_MAKEFLAGS%" == "fall _RELEASE 2" goto SKIPCLEAN
if not '%_CLEANMAKE%' == '1' goto SKIPCLEAN
copy %1\make.bat %_flag%.tmp >nul
attrib -h %1\slm.ini
copy %1\slm.ini %_flag%.slm >nul
rd /q/s %1
md %1 >nul
copy %_flag%.tmp %1\make.bat >nul
copy %_flag%.slm %1\slm.ini >nul
attrib +r %1\make.bat >nul
attrib +h +r %1\slm.ini >nul
del /f /q %_flag%.tmp >nul
del /f /q %_flag%.slm >nul

:SKIPCLEAN
if '%_DROP%' == '0' GOTO SKIPDBSTUFF
if not exist %_F3DIR%\build\%1\log md %_F3DIR%\build\%1\log

set | %!AWK% "/^_+/ {print}" >%!SETLOG%
if not exist %_DESTROOT%\%_DROPNAME%\%2 md %_DESTROOT%\%_DROPNAME%\%2
if not exist %_DESTROOT%\%_DROPNAME%\%2\%3 md %_DESTROOT%\%_DROPNAME%\%2\%3
if not exist %_DESTROOT%\%_DROPNAME%\%2\%3\log md %_DESTROOT%\%_DROPNAME%\%2\%3\log
xcopy %_F3DIR%\build\%1\log\*.* %_DESTROOT%\%_DROPNAME%\%2\%3\log /e/i/v

rem ********RUN MAKE************************************************
:SKIPDBSTUFF
cd %_F3DIR%\build\%1
%!NOW% ---------------------------- Starting %1 Build >%!BUILDLOG%
call make %_MAKEFLAGS% >>%!BUILDLOG%
%!NOW% ---------------------------- Finished %1 Build >>%!BUILDLOG%

rem *******SCAN BUILD.LOG FOR ERRORS********************************
title Checking %1
%!PERL% %!F3BUILDDIR%\iserror.pl %!BUILDLOG% %!ERRORLOG%
set !errors=No
set !complete=Yes
if not exist %_F3DIR%\BUILD\%1\bin\MSHTML.dll set !complete=No
if not exist %_F3DIR%\BUILD\%1\bin\MSHTMpad.exe set !complete=No
if not exist %_F3DIR%\BUILD\%1\bin\MSHTMenu.dll set !complete=No

if not exist %!ERRORLOG% goto NOFAILMAIL

REM *******BUILD FAILURE MAIL**************************************
set !errors=Yes

set mm=%!MAILPATH%\%_DROPNAME%.%_flag%
if '%_MAIL%' == '0' goto NOFAILMAIL

copy %!ERRORLOG% %mm%
echo. >>%mm%
if '%!complete%' == 'No' echo Build did not complete!>>%mm%
echo (Results are from drop %_DROPNAME%)>>%mm%
echo %_SENDFAIL% /S Trident %1 build problems!>>%mm%

REM *****SKIP TO HERE IF RESULTS ARE GOOD***************************
:NOFAILMAIL

rem ******MAKE RESULTS.LOG FOR MAIL SEND****************************
echo %2\%3		%!complete%		%!errors% >>%!BLDRES%

rem ******IF NOT DROPPING, WE'RE DONE*******************************
if '%_DROP%' == '0' GOTO END

rem Drop results but not product if it wasn't complete-
if '%!complete%' == 'No' goto END
title Dropping %1
call %!F3BUILDDIR%\drop %1 %2 %3>%!DROPLOG%
echo set _CURRENT_F3_VER=%_DROPNAME%>%_DESTROOT%\curf3ver.bat

REM *****************************************

rem DRT Trigger
if NOT '%_TP3%' == '1' goto END

set !TRIGGERFORMAT=TRIDENT
if '%3' == 'ship' set !TRIGGERFORMAT=S%!TRIGGERFORMAT%
if '%3' == 'debug' set !TRIGGERFORMAT=D%!TRIGGERFORMAT%
echo Token: $$_%!TRIGGERFORMAT%=%_DROPNAME%>\\triatcm\tp3\triggers\drt\GODRT%_flag%.txt

echo ------------->>\\triatcm\tp3\triggers\drt\check\GODRT%_flag%.txt
%!NOW% >>\\triatcm\tp3\triggers\drt\check\GODRT%_flag%.txt
echo Token: $$_%!TRIGGERFORMAT%=%_DROPNAME%>>\\triatcm\tp3\triggers\drt\check\GODRT%_flag%.txt

:END
REM ********REMOVE BUILDDONE TO SIGNAL BUILD.BAT*******************
if exist %!BUILDDONE%.%_FLAG% del /f /q %!BUILDDONE%.%_FLAG%
if exist %!ERRORLOG% type %!ERRORLOG%
popd
endlocal