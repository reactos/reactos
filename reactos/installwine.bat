@echo off
REM
REM created by sedwards 11/11/01
REM
if "%1" == "" goto NoParameter
set WINE_INSTALL=%1
goto Install
:NoParameter

set ROS_INSTALL=c:\reactos
set WINE_INSTALL=%ROS_INSTALL%\system32
set WINE_APP_INSTALL=%ROS_INSTALL%\bin
set WINE_TESTS_INSTALL=%ROS_INSTALL%\tests


:Install
echo Installing libwine and wine_unicode to %WINE_INSTALL%
copy ..\wine\library\libwine.dll %WINE_INSTALL%
copy ..\wine\unicode\wine_unicode.dll %WINE_INSTALL%


echo Installing dlls to %WINE_INSTALL%
@echo off
copy ..\wine\dlls\comctl32\comctl32.dll %WINE_INSTALL%
copy ..\wine\dlls\commdlg\comdlg32.dll  %WINE_INSTALL%
copy ..\wine\dlls\ddraw\ddraw.dll 	%WINE_INSTALL%
copy ..\wine\dlls\dinput\dinput.dll 	%WINE_INSTALL%
copy ..\wine\dlls\dplay\dplay.dll 	%WINE_INSTALL%
copy ..\wine\dlls\dplayx\dplayx.dll 	%WINE_INSTALL%
copy ..\wine\dlls\mapi32\mapi32.dll 	%WINE_INSTALL%
copy ..\wine\dlls\mpr\mpr.dll 		%WINE_INSTALL%
copy ..\wine\dlls\netapi32\netapi32.dll %WINE_INSTALL%
copy ..\wine\dlls\odbc32\odbc32.dll 	%WINE_INSTALL%
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
copy ..\wine\dlls\tapi32\tapi32.dll	%WINE_INSTALL%
copy ..\wine\dlls\urlmon\urlmon.dll	%WINE_INSTALL%
REMcopy ..\wine\dlls\version\version.dll	%WINE_INSTALL%
copy ..\wine\dlls\wintrust\wintrust.dll	%WINE_INSTALL%
copy ..\wine\dlls\winspool\winspool.drv	%WINE_INSTALL%
REM
echo Installing winelib programs to %WINE_APP_INSTALL%
REM
copy ..\wine\programs\clock\winclock.exe		%WINE_APP_INSTALL%
copy ..\wine\programs\cmdlgtst\cmdlgtst.exe		%WINE_APP_INSTALL%
copy ..\wine\programs\control\control.exe		%WINE_APP_INSTALL%
copy ..\wine\programs\notepad\notepad.exe		%WINE_APP_INSTALL%
copy ..\wine\programs\progman\progman.exe		%WINE_APP_INSTALL%
copy ..\wine\programs\uninstaller\uninstaller.exe	%WINE_APP_INSTALL%
copy ..\wine\programs\view\view.exe			%WINE_APP_INSTALL%
copy ..\wine\programs\wcmd\wcmd.exe			%WINE_APP_INSTALL%
copy ..\wine\programs\winefile\winefile.exe		%WINE_APP_INSTALL%
copy ..\wine\programs\winemine\winmine.exe		%WINE_APP_INSTALL%
copy ..\wine\programs\winver\winver.exe			%WINE_APP_INSTALL%
REM
echo Installing wine tools to %WINE_APP_INSTALL%
REM
copy ..\wine\tools\winedump\winedump.exe		%WINE_APP_INSTALL%

REM
echo Installing Regression tests to %WINE_TESTS_INSTALL%
REM
copy ..\wine\dlls\advapi32\tests\advapi32_test.exe	%WINE_TESTS_INSTALL%
copy ..\wine\dlls\kernel\tests\kernel32_test.exe	%WINE_TESTS_INSTALL%
copy ..\wine\dlls\ntdll\tests\ntdll_test.exe		%WINE_TESTS_INSTALL%
copy ..\wine\dlls\user32\tests\user32_test.exe		%WINE_TESTS_INSTALL%
REM
pause
