@echo off
if "%1" == "" goto NoParameter
set ROS_INSTALL=%1
goto Install
:NoParameter
set ROS_INSTALL=c:\reactos
:Install
echo Installing ReactOS user programs to %ROS_INSTALL%\bin
@echo off

copy cmd\cmd.exe		%ROS_INSTALL%\bin
copy dflat32\edit.exe		%ROS_INSTALL%\bin
copy mc\release\mc.exe		%ROS_INSTALL%\bin
copy net\finger\finger.exe 	%ROS_INSTALL%\bin
copy net\ncftp\ncftp.exe 	%ROS_INSTALL%\bin
copy net\ping\ping.exe		%ROS_INSTALL%\bin
copy net\telnet\telnet.exe 	%ROS_INSTALL%\bin
copy net\telnet\telnet.cfg 	%ROS_INSTALL%\bin
copy net\telnet\telnet.ini 	%ROS_INSTALL%\bin
copy net\whois\whois.exe 	%ROS_INSTALL%\bin
copy notevil\notevil.exe	%ROS_INSTALL%\bin
copy sysutils\chkdsk.exe	%ROS_INSTALL%\bin
copy sysutils\chklib.exe	%ROS_INSTALL%\bin
copy sysutils\format.exe	%ROS_INSTALL%\bin
copy sysutils\ldd.exe		%ROS_INSTALL%\bin
copy sysutils\pedump.exe	%ROS_INSTALL%\bin
copy regedit\regedit.exe	%ROS_INSTALL%\bin
copy regedt32\regedt32.exe	%ROS_INSTALL%\bin
copy taskmgr\taskmgr.exe	%ROS_INSTALL%\bin
