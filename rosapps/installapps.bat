@echo off
if "%1" == "" goto NoParameter
set ROS_INSTALL=%1
goto Install
:NoParameter
set ROS_INSTALL=c:\reactos
:Install
echo on
echo Installing winelib programs to %ROS_INSTALL%\bin
@echo off

copy cmd\cmd.exe		%ROS_INSTALL%\bin
copy mc\release\mc.exe		%ROS_INSTALL%\bin
copy net\ncftp\ncftp.exe 	%ROS_INSTALL%\bin
copy net\ping\ping.exe		%ROS_INSTALL%\bin
copy net\telnet\telnet.exe 	%ROS_INSTALL%\bin
copy notevil\notevil.exe	%ROS_INSTALL%\bin
copy sysutils\chkdsk.exe	%ROS_INSTALL%\bin
copy sysutils\chklib.exe	%ROS_INSTALL%\bin
copy sysutils\format.exe	%ROS_INSTALL%\bin
copy sysutils\ldd.exe		%ROS_INSTALL%\bin
copy sysutils\pedump.exe	%ROS_INSTALL%\bin
