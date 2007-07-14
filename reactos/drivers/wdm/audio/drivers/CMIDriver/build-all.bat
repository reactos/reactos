@echo off
start /wait cmd.exe /c distclean.bat
start /wait cmd.exe /c build-release-x64.bat
start /wait cmd.exe /c build-release-x64-WaveRT.bat
start /wait cmd.exe /c build-release-x86.bat
start /wait cmd.exe /c build-release-x86-WaveRT.bat                    