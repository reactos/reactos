@echo off
REM
REM created by sedwards 11/11/01
REM
if "%1" == "" goto NoParameter
set WINE_INSTALL=%1
goto Install
:NoParameter
set WINE_INSTALL=c:\reactos\system32
set ROS_INSTALL=c:\reactos
:Install
echo Installing dlls to %WINE_INSTALL%
@echo off
copy ..\wine\dlls\comctl32\comctl32.dll %WINE_INSTALL%
copy ..\wine\dlls\commdlg\comdlg32.dll  %WINE_INSTALL%
copy ..\wine\dlls\ddraw\ddraw.dll 	%WINE_INSTALL%
copy ..\wine\dlls\dinput\dinput.dll 	%WINE_INSTALL%
copy ..\wine\dlls\dplay\dplay.dll 	%WINE_INSTALL%
copy ..\wine\dlls\dplayx\dplayx.dll 	%WINE_INSTALL%
copy ..\wine\dlls\mapi32\mapi32.dll 	%WINE_INSTALL%
copy ..\wine\dlls\ole32\ole32.dll 	%WINE_INSTALL%
copy ..\wine\dlls\oleaut32\oleaut32.dll %WINE_INSTALL%
copy ..\wine\dlls\olecli\olecli32.dll   %WINE_INSTALL%
copy ..\wine\dlls\oledlg\oledlg.dll 	%WINE_INSTALL%
copy ..\wine\dlls\olepro32\olepro32.dll %WINE_INSTALL%
copy ..\wine\dlls\olesvr\olesvr32.dll 	%WINE_INSTALL%
copy ..\wine\dlls\psapi\psapi.dll 	%WINE_INSTALL%
copy ..\wine\dlls\richedit\riched32.dll	%WINE_INSTALL%
copy ..\wine\dlls\rpcrt4\rpcrt4.dll	%WINE_INSTALL%
copy ..\wine\dlls\serialui\serialui.dll	%WINE_INSTALL%
copy ..\wine\dlls\shdocvw\shdocvw.dll	%WINE_INSTALL%
copy ..\wine\dlls\shell32\shell32.dll	%WINE_INSTALL%
copy ..\wine\dlls\shfolder\shfolder.dll	%WINE_INSTALL%
copy ..\wine\dlls\shlwapi\shlwapi.dll	%WINE_INSTALL%
copy ..\wine\dlls\urlmon\urlmon.dll	%WINE_INSTALL%
REM
echo Installing winelib programs to %ROS_INSTALL%\bin
REM
copy ..\wine\programs\clock\winclock.exe		%ROS_INSTALL%\bin
copy ..\wine\programs\cmdlgtst\cmdlgtst.exe		%ROS_INSTALL%\bin
copy ..\wine\programs\control\control.exe		%ROS_INSTALL%\bin
copy ..\wine\programs\notepad\notepad.exe		%ROS_INSTALL%\bin
copy ..\wine\programs\progman\progman.exe		%ROS_INSTALL%\bin
copy ..\wine\programs\uninstaller\uninstaller.exe	%ROS_INSTALL%\bin
copy ..\wine\programs\view\view.exe			%ROS_INSTALL%\bin
copy ..\wine\programs\wcmd\wcmd.exe			%ROS_INSTALL%\bin
copy ..\wine\programs\winefile\winefile.exe		%ROS_INSTALL%\bin
copy ..\wine\programs\winemine\winmine.exe		%ROS_INSTALL%\bin
copy ..\wine\programs\winver\winver.exe			%ROS_INSTALL%\bin



