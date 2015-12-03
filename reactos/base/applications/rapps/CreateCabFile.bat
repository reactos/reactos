@echo off

cd ..\..\..\media

mkdir rapps\utf16

for %%f in (rapps\*.txt) do (
     ..\output-MinGW-i386\host-tools\utf16le.exe "rapps\%%~nf.txt" "rapps\utf16\%%~nf.txt"
)

..\output-MinGW-i386\host-tools\cabman.exe -M mszip -S rapps\rappmgr.cab rapps\utf16\*.txt
rmdir /s /q rapps\utf16
