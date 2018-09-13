@echo off
@echo Remove the Font Support diskette from the diskette drive.
@echo The Control Panel International applet will be started now.  Please
@echo select the Country, Language and Keyboard which you would like to have
@echo set as the system defaults.  Once you have made the appropriate
@echo selections, choose the "Close" button.  Once the International applet
@echo is closed, please close the Control Panel window for completion of
@echo the Font Support installation.
pause
call %systemroot%\system32\control INTERNATIONAL
pause
call %systemroot%\system32\sublocal -userdef
@echo Application fonts for the additional locales have been placed
@echo in directories under %systemroot%\system\appfonts.  You may install
@echo these fonts using the Control Panel Fonts applet, choosing "Add"
@echo and choosing the correct directory.  The subdirectories are:
@echo %systemroot\system\appfonts\Cyrillic
@echo                            \TURKISH
@echo                            \EasternEurope
@echo The Font Support installation is now complete.
@echo Please reboot at this time for the new locale to be activated.
pause
exit
