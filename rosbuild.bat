::
:: This script is called from the Makefile command line within Visual Studio using the following parameters:
::
::		%1 - $(build)
::		%2 - $(target)
::
:: Examples:
::
::		Call build.bat build ntoskrnl
::		Call build.bat clean win32k
::

@echo off

if "%1"=="" goto :err_params
if "%2"=="" goto :err_params


:: Get the RosBE path... ::

:: Set the command we'll use to check if RosBE exists
set _IS_ROSBE_INSTALLED_COMMAND="reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Uninstall\ReactOS Build Environment for Windows" /v UninstallString"

:: Check the key actually exists !!!!FIXME: Why is this returning 'The system cannot find the path specified.'!!!!
%_IS_ROSBE_INSTALLED_COMMAND%
IF NOT errorlevel 0 goto :err_no_rosbe

:: This is a bit hackish. What we do is look for REG_SZ which is the second token on the second line and dump it into i
:: We then assign all remaining text to the next variable in the sequence, which is j. This leaves us with the path
for /F "tokens=2,* skip=1 delims=\ " %%i in ('%_IS_ROSBE_INSTALLED_COMMAND%') do (
    set _ROSBE_UNINSTALL_PATH_=%%j
)


:: Now strip the file name from the end of the path and we should have our RosBE install directory
set _ROSBE_PATH_DIR=
set _ROSBE_PATH_=
for %%i in ("%_ROSBE_UNINSTALL_PATH_%") do set _ROSBE_PATH_DIR=%%~di
for %%i in ("%_ROSBE_UNINSTALL_PATH_%") do set "_ROSBE_PATH_=%%~pi"
set "_ROSBE_FULL_PATH_=%_ROSBE_PATH_DIR%%_ROSBE_PATH_%"
::echo RosBE insall path = %_ROSBE_FULL_PATH_%

:: Set the path which contains our build tools
set _ROSBE_BIN_PATH=%_ROSBE_FULL_PATH_%Tools

:: Add the path to the search path
path=%path%;%_ROSBE_BIN_PATH%

:: Set the make path
set _MAKE_COMMAND=""
if exist "%_ROSBE_BIN_PATH%\mingw32-make.exe" (
    set _MAKE_COMMAND=mingw32-make.exe
)
if exist "%_ROSBE_BIN_PATH%\make.exe" (
    set _MAKE_COMMAND=make.exe
)
if %_MAKE_COMMAND% == "" (
    goto err_no_make
)

:: This file is located in the source root
set _ROS_SOURCE_ROOT=%~dp0

:: Change the current dir to the source root
cd %_ROS_SOURCE_ROOT%

:: Run the requested build task
if "%1" == "build" (
    goto :build
) 
if "%1" == "rebuild" (
    goto clean
)
if "%1" == "clean" (
    goto :clean
)
goto :err_params

:clean
echo.
echo Cleaning...
echo.
call "%_MAKE_COMMAND%" -j 1 %2%_clean

if "%1" == "rebuild" (
    goto :build
)

goto :exit

:build
echo.
echo Building...
echo.
call "%_MAKE_COMMAND%" -j 1 %2%

goto :exit

:err_no_make
echo.
echo Cannot find  a make executable
goto :err_no_rosbe

:err_no_rosbe
echo.
echo You need to have RosBE installed to use this configuration
echo.
exit 1

:err_params
echo. 
echo Invalid parameters required, Check your command line.
echo.
exit 2

:exit
echo.
