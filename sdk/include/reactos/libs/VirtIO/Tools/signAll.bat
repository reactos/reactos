@echo off
SET "SignToolDir=C:\Program Files (x86)\Windows Kits\8.1\bin\x64"
for /r "%~dp0\..\" %%i in (*.sys) do "%SignToolDir%\signtool.exe" sign /f "%~dp0\VirtIOTestCert.pfx" "%%i"
for /r "%~dp0\..\" %%i in (*.cat) do "%SignToolDir%\signtool.exe" sign /f "%~dp0\VirtIOTestCert.pfx" "%%i"

