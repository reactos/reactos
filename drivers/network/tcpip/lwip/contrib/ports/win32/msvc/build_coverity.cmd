@echo off
rem Usage: pass the path to cov-build.exe (with trailing backslash, without the exe) as first parameter
rem ATTENTION: this deletes the output folder "cov-int" and the output file "cov-int.zip" first!

set devenv="%ProgramFiles%\Microsoft Visual Studio 10.0\Common7\IDE\devenv.exe"
if not exist %devenv% set devenv="%ProgramFiles%\Microsoft Visual Studio 10.0\Common7\IDE\vcexpress.exe"
if not exist %devenv% set devenv="%ProgramFiles(x86)%\Microsoft Visual Studio 10.0\Common7\IDE\devenv.exe"
if not exist %devenv% set devenv="%ProgramFiles(x86)%\Microsoft Visual Studio 10.0\Common7\IDE\vcexpress.exe"
set covbuild=%1cov-build.exe
set covoutput=cov-int
set zip7="c:\Program Files\7-Zip\7z.exe"

pushd %~dp0

if exist %covoutput% rd /s /q %covoutput%
if exist %covoutput%.zip del %covoutput%.zip

%covbuild% --dir %covoutput% %devenv% lwip_test.sln /build Debug || goto error

if exist %zip7% goto dozip
echo error: 7zip not found at \"%zip7%
goto error
:dozip
%zip7% a %covoutput%.zip %covoutput%
:error
popd