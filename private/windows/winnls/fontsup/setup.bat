@echo off
rem This batch file will enable a previously installed Windows NT 3.5 system
rem to use the correct fonts for all non-code page 1252 locales.
rem
@echo If your 3.5" floppy drive is B:, please enter SETUP B:
pause
set flpydrv=
set wrk_srv=
set flpydrv=%1
if "%1"=="" (if exist a:\fontsup.bat set flpydrv=a:)
if "%flpydrv%"=="" goto nofloppy
@echo Copying font files....
call expand %flpydrv%\system\*.fon %systemroot%\system >%systemroot%\fontsup.log
call expand %flpydrv%\%processor_architecture%\kbdsel.ex_ %systemroot%\system32\kbdsel.exe >>%systemroot%\fontsup.log
call expand %flpydrv%\%processor_architecture%\kbddll.dl_ %systemroot%\system32\kbddll.dll >>%systemroot%\fontsup.log


if exist %systemroot%\lanmannt.bmp set wrk_srv=srv
if "%wrk_srv%"=="" set wrk_srv=wrk
if "%wrk_srv%"=="wrk" call expand %flpydrv%\%processor_architecture%\language.in_ %systemroot%\system32\language.inf >>%systemroot%\fontsup.log
if "%wrk_srv%"=="srv" call expand %flpydrv%\%processor_architecture%\language.sr_ %systemroot%\system32\language.inf >>%systemroot%\fontsup.log
call expand %flpydrv%\system32\registry.in_ %systemroot%\system32\registry.inf >>%systemroot%\fontsup.log
call expand %flpydrv%\%PROCESSOR_ARCHITECTURE%\sublocal.ex_ %systemroot%\system32\sublocal.exe >>%systemroot%\fontsup.log
if not exist %systemroot%\system\appfonts md %systemroot%\system\appfonts >>%systemroot%\fontsup.log
if not exist %systemroot%\system\appfonts\Cyrillic md %systemroot%\system\appfonts\Cyrillic >>%systemroot%\fontsup.log
if not exist %systemroot%\system\appfonts\TURKISH md %systemroot%\system\appfonts\TURKISH >>%systemroot%\fontsup.log
if not exist %systemroot%\system\appfonts\EasternEurope md %systemroot%\system\appfonts\EasternEurope >>%systemroot%\fontsup.log
call expand %flpydrv%\appfonts\serife.cyr %systemroot%\system\appfonts\Cyrillic\serife.fon >>%systemroot%\fontsup.log
call expand %flpydrv%\appfonts\coure.cyr %systemroot%\system\appfonts\Cyrillic\coure.fon >>%systemroot%\fontsup.log
call expand %flpydrv%\appfonts\ARIAL.tur %systemroot%\system\appfonts\TURKISH\ARIAL.fon >>%systemroot%\fontsup.log
call expand %flpydrv%\appfonts\COUR.tur %systemroot%\system\appfonts\TURKISH\COUR.fon >>%systemroot%\fontsup.log
call expand %flpydrv%\appfonts\TIMES.tur %systemroot%\system\appfonts\TURKISH\TIMES.fon >>%systemroot%\fontsup.log
call expand %flpydrv%\appfonts\ARIAL.ee %systemroot%\system\appfonts\EasternEurope\ARIAL.fon >>%systemroot%\fontsup.log
call expand %flpydrv%\appfonts\COUR.ee %systemroot%\system\appfonts\EasternEurope\COUR.fon >>%systemroot%\fontsup.log
call expand %flpydrv%\appfonts\TIMES.ee %systemroot%\system\appfonts\EasternEurope\TIMES.fon >>%systemroot%\fontsup.log
copy %flpydrv%\fontsup.bat %systemroot%\system32\fontsup.bat >>%systemroot%\fontsup.log
%homedrive%
%systemroot%\system32\fontsup.bat
:nofloppy
@echo Please specify the floppy drive letter
@echo when invoking SETUP.BAT, i.e., SETUP B:
pause
