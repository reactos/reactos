@echo off
rem   []------[ReactOS MAN Project ]--------[]
rem      Project:     ReactOS manual browser
rem      File:        man.cmd
rem      Purpose:     Clone of UNIX man
rem      Programmers: Semyon Novikov
rem      Version:     0.1.2
rem      OS:          WinNT/ReactOS/os2 eCs(testing)
rem      License:     GPL
rem   []------------------------------------[]


rem []==[Config area]==[]
set MANED=edit
set MANMORE=cat
set MAN=%WINDIR%\man
rem []==[End of config area]==[]

goto chk_param

:chk_param

 if "%4"=="/create" attrib -r %MAN%\%SECTION%\%1.man
 if "%4"=="/create" %ED% %MAN%\%SECTION%\%1.man
 if "%4"=="/create" goto end

 if "%2"=="/e" set ED=%MANED%
 if "%2"=="/e" goto locate

 if "%3"=="/e" set ED=%MANED%
 if "%3"=="/e" goto chk_section

 if "%2"=="" set ED=%MANMORE%
 if "%2"=="" goto locate

:chk_section
 set SECTION=%2
 set ED=%MANMORE%
 if "%3"=="/e" set ED=%MANED%
goto open_page

:locate
 if exist %MAN%\1\%1.man set SECTION=1
 if exist %MAN%\2\%1.man set SECTION=2
 if exist %MAN%\3\%1.man set SECTION=3
 if exist %MAN%\4\%1.man set SECTION=4
 if exist %MAN%\5\%1.man set SECTION=5

:open_page
if not exist %MAN%\%SECTION%\%1.man echo No manual for %1
if exist %MAN%\%SECTION%\%1.man cls
if exist %MAN%\%SECTION%\%1.man %ED% %MAN%\%SECTION%\%1.man

:end