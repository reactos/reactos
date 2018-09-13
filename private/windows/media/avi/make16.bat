set PROJECT=VFW
set RT=%_NTDRIVE%\nt\private\windows\media\avi
set WANT_286=1


if not exist %RT%\mciavi32\vfw16\debug\nul         md %RT%\mciavi32\vfw16\debug
if not exist %RT%\mciavi32\vfw16\debug\inc.16\nul  md %RT%\mciavi32\vfw16\debug\inc.16
if not exist %RT%\mciavi32\vfw16\debug\lib.16\nul  md %RT%\mciavi32\vfw16\debug\lib.16

if not exist %RT%\mciavi32\vfw16\retail\nul        md %RT%\mciavi32\vfw16\retail
if not exist %RT%\mciavi32\vfw16\retail\inc.16\nul md %RT%\mciavi32\vfw16\retail\inc.16
if not exist %RT%\mciavi32\vfw16\retail\lib.16\nul md %RT%\mciavi32\vfw16\retail\lib.16


rem
rem FIRST do compman
rem
cd compman.16
set NTDEBUG=cvp
nmake -a
set NTDEBUG=
nmake -a

rem
rem Now do drawdib
rem
cd ..\drawdib.16
set NTDEBUG=cvp
nmake -a
set NTDEBUG=
nmake -a


rem
rem Now do mciwnd
rem
cd ..\mciwnd.16
set NTDEBUG=cvp
nmake -a
set NTDEBUG=
nmake -a


rem
rem Now do msvideo
rem
cd ..\msvideo.16
set NTDEBUG=cvp
nmake -a
set NTDEBUG=
nmake -a



rem
rem Now do avicap
rem
cd ..\avicap.16
del *.obj /s
nmake


rem
rem Now do avifile
rem
cd ..\avifile.16
set NTDEBUG=cvp
nmake -a
set NTDEBUG=
nmake -a

rem
rem Now do mciavi
rem
cd ..\mciavi.16
del *.obj /s
nmake

set PROJECT=
