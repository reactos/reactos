@echo off

REM this batch file copies the Access Utility for Windows NT 
REM and documentation to the System32 directory located under
REM your Windows NT directory.
if not exist access.exe goto wrongdir

:rightdir
echo Copying ACCESS.EXE to %SystemRoot%\System32\ACCESS.EXE ...
copy /b /v access.exe %SystemRoot%\System32
echo Copying ACCESS35.HLP to %SystemRoot%\System32\ACCESS35.HLP ...
copy /b /v access35.hlp %SystemRoot%\System32
echo Copying SKEYS.EXE to %SystemRoot%\System32\SKEYS.EXE ...
copy /b /v skeys.exe %SystemRoot%\System32
echo Copying SKDLL.DLL to %SystemRoot%\System32\SKDLL.DLL ...
copy /b /v skdll.dll %SystemRoot%\System32
echo Copying ACCESS.WRI to %SystemRoot%\System32\ACCESS35.WRI ...
copy /b /v access.wri %SystemRoot%\System32

echo ...
echo The Access Utility is now installed and can be run by choosing
echo the Run command from Program Manager's File menu and typing "ACCESS".
echo For more information on using the Access Utility, read ACCESS.WRI
echo using Microsoft Windows Write, or use the on-line help available
echo from within the Access Utility.
echo ...
pause
goto egress

:wrongdir
echo This batch file will copy the Access Utility for Windows NT
echo and accompanying documentation to your Windows NT directory.
echo You must run this batch file from the location of ACCESS.EXE.
pause
goto egress

:egress
