@echo off
if "%1" == "" goto NoParameter
set ROS_INSTALL=%1
goto Install
:NoParameter
set ROS_INSTALL=c:\reactos
:Install
echo Installing to %ROS_INSTALL%
@echo off

md %ROS_INSTALL%
md %ROS_INSTALL%\bin
md %ROS_INSTALL%\symbols
md %ROS_INSTALL%\system32

copy ..\os2\apps\bepslep\bepslep.exe %ROS_INSTALL%\bin
copy ..\os2\lib\doscalls\doscalls.dll %ROS_INSTALL%\system32
copy ..\os2\lib\doscalls\doscalls.sym %ROS_INSTALL%\symbols
copy ..\os2\server\os2ss.exe %ROS_INSTALL%\system32
copy ..\os2\server\os2ss.sym %ROS_INSTALL%\symbols
