@echo off
if "%1" == "" goto NoParameter
set ROS_INSTALL=%1
goto Install
:NoParameter
set ROS_INSTALL=c:\reactos
:Install
echo Installing ReactOS user programs to %ROS_INSTALL%\bin
@echo off

copy cmd\cmd.exe			%ROS_INSTALL%\bin
copy calc\calc.exe			%ROS_INSTALL%\bin
copy cmdutils\tee.exe			%ROS_INSTALL%\bin
copy cmdutils\more.exe			%ROS_INSTALL%\bin
copy cmdutils\y.exe			%ROS_INSTALL%\bin
copy cmdutils\touch\touch.exe		%ROS_INSTALL%\bin
copy control\control.exe		%ROS_INSTALL%\bin
copy ctlpanel\roscfg\roscfg.cpl		%ROS_INSTALL%\system32
copy ctlpanel\rospower\rospower.cpl	%ROS_INSTALL%\system32
copy dflat32\edit.exe			%ROS_INSTALL%\bin
copy hcalc\hcalc.exe			%ROS_INSTALL%\bin
copy mc\release\mc.exe			%ROS_INSTALL%\bin
copy net\arp\arp.exe	 		%ROS_INSTALL%\bin
copy net\echo\echo.exe	 		%ROS_INSTALL%\bin
copy net\finger\finger.exe 		%ROS_INSTALL%\bin
copy net\ipconfig\ipconfig.exe 		%ROS_INSTALL%\bin
copy net\ipecho\ipecho.exe 		%ROS_INSTALL%\bin
copy net\ncftp\ncftp.exe 		%ROS_INSTALL%\bin
copy net\netstat\netstat.exe 		%ROS_INSTALL%\bin
copy net\niclist\niclist.exe 		%ROS_INSTALL%\bin
copy net\ping\ping.exe			%ROS_INSTALL%\bin
copy net\route\route.exe 		%ROS_INSTALL%\bin
copy net\telnet\telnet.exe 		%ROS_INSTALL%\bin
copy net\telnet\telnet.cfg 		%ROS_INSTALL%\bin
copy net\telnet\telnet.ini 		%ROS_INSTALL%\bin
copy net\whois\whois.exe 		%ROS_INSTALL%\bin
copy notevil\notevil.exe		%ROS_INSTALL%\bin
copy sysutils\chkdsk.exe		%ROS_INSTALL%\bin
copy sysutils\chklib.exe		%ROS_INSTALL%\bin
copy sysutils\format.exe		%ROS_INSTALL%\bin
copy sysutils\ldd.exe			%ROS_INSTALL%\bin
copy sysutils\pedump.exe		%ROS_INSTALL%\bin
copy sysutils\regexpl\regexpl.exe	%ROS_INSTALL%\bin
copy sysutils\tlist\tlist.exe		%ROS_INSTALL%\bin
copy regedit\regedit.exe		%ROS_INSTALL%\bin
copy regedt32\regedt32.exe		%ROS_INSTALL%\bin
copy taskmgr\taskmgr.exe		%ROS_INSTALL%\bin
copy winfile\winfile.exe		%ROS_INSTALL%\bin
