@echo off
if not "%VSFLAVOR%"=="" goto :knownVS
call :checkvs
echo USING %VSFLAVOR% Visual Studio

:knownVS
echo %0: Setting NATIVE ENV for %1 (VS %VSFLAVOR%)...
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\%VSFLAVOR%\VC\Auxiliary\Build\vcvarsall.bat" %1
goto :eof

:checkvs
set VSFLAVOR=Professional
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" set VSFLAVOR=Community
goto :eof
