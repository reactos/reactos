@echo off
if "%1" == "" goto NoParameter
set ROS_INSTALL=%1
goto Install
:NoParameter
set ROS_INSTALL=c:\reactos
:Install
echo Installing ReactOS user programs to %ROS_INSTALL%\bin
@echo off

copy ..\rosapps\cmd\cmd.exe			%ROS_INSTALL%\bin
copy ..\rosapps\calc\calc.exe			%ROS_INSTALL%\bin
copy ..\rosapps\cmdutils\tee.exe		%ROS_INSTALL%\bin
copy ..\rosapps\cmdutils\more.exe		%ROS_INSTALL%\bin
copy ..\rosapps\cmdutils\y.exe			%ROS_INSTALL%\bin
copy ..\rosapps\cmdutils\mode\mode.exe		%ROS_INSTALL%\bin
copy ..\rosapps\cmdutils\touch\touch.exe	%ROS_INSTALL%\bin
copy ..\rosapps\control\control.exe		%ROS_INSTALL%\bin
copy ..\rosapps\ctlpanel\roscfg\roscfg.cpl	%ROS_INSTALL%\system32
copy ..\rosapps\ctlpanel\rospower\rospower.cpl	%ROS_INSTALL%\system32
copy ..\rosapps\dflat32\edit.exe		%ROS_INSTALL%\bin
copy ..\rosapps\hcalc\hcalc.exe			%ROS_INSTALL%\bin
copy ..\rosapps\mc\release\mc.exe		%ROS_INSTALL%\bin
copy ..\rosapps\net\arp\arp.exe	 		%ROS_INSTALL%\bin
copy ..\rosapps\net\echo\echo.exe	 	%ROS_INSTALL%\bin
copy ..\rosapps\net\finger\finger.exe 		%ROS_INSTALL%\bin
copy ..\rosapps\net\ipconfig\ipconfig.exe 	%ROS_INSTALL%\bin
copy ..\rosapps\net\ipecho\ipecho.exe 		%ROS_INSTALL%\bin
copy ..\rosapps\net\ncftp\ncftp.exe 		%ROS_INSTALL%\bin
copy ..\rosapps\net\netstat\netstat.exe 	%ROS_INSTALL%\bin
copy ..\rosapps\net\niclist\niclist.exe 	%ROS_INSTALL%\bin
copy ..\rosapps\net\ping\ping.exe		%ROS_INSTALL%\bin
copy ..\rosapps\net\route\route.exe 		%ROS_INSTALL%\bin
copy ..\rosapps\net\telnet\telnet.exe 		%ROS_INSTALL%\bin
copy ..\rosapps\net\telnet\telnet.cfg 		%ROS_INSTALL%\bin
copy ..\rosapps\net\telnet\telnet.ini 		%ROS_INSTALL%\bin
copy ..\rosapps\net\whois\whois.exe 		%ROS_INSTALL%\bin
copy ..\rosapps\notevil\notevil.exe		%ROS_INSTALL%\bin
copy ..\rosapps\sysutils\chkdsk.exe		%ROS_INSTALL%\bin
copy ..\rosapps\sysutils\chklib.exe		%ROS_INSTALL%\bin
copy ..\rosapps\sysutils\format.exe		%ROS_INSTALL%\bin
copy ..\rosapps\sysutils\ldd.exe		%ROS_INSTALL%\bin
copy ..\rosapps\sysutils\pedump.exe		%ROS_INSTALL%\bin
copy ..\rosapps\sysutils\regexpl\regexpl.exe	%ROS_INSTALL%\bin
copy ..\rosapps\sysutils\tlist\tlist.exe	%ROS_INSTALL%\bin
copy ..\rosapps\regedit\regedit.exe		%ROS_INSTALL%\bin
copy ..\rosapps\regedt32\regedt32.exe		%ROS_INSTALL%\bin
copy ..\rosapps\taskmgr\taskmgr.exe		%ROS_INSTALL%\bin
copy ..\rosapps\winfile\winfile.exe		%ROS_INSTALL%\bin
