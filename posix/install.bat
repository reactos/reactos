@echo off
if "%1" == "" goto NoParameter
set ROS_INSTALL=%1
goto Install
:NoParameter
set ROS_INSTALL=c:\reactos
:Install
echo on
echo Installing to %ROS_INSTALL%
@echo off

md %ROS_INSTALL%
md %ROS_INSTALL%\bin
md %ROS_INSTALL%\symbols
md %ROS_INSTALL%\system32

copy apps\baresh\baresh.exe %ROS_INSTALL%\bin
copy apps\posixw32\posixw32.exe %ROS_INSTALL%\bin
copy server\psxss.exe %ROS_INSTALL%\system32
copy lib\psxdll\psxdll.dll %ROS_INSTALL%\system32
copy lib\psxdll\psxdll.map %ROS_INSTALL%\symbols
copy lib\psxx\psxx.dll %ROS_INSTALL%\system32
copy lib\psxx\psxx.map %ROS_INSTALL%\symbols

