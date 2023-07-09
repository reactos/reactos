@echo off
rem
rem undname\bvt.bat - a bvt for the undecorator
rem This batch file builds and then runs the undecorator bvt (testundn.cxx).
rem

echo Building testundn.exe... | tee testundn.log
cl testundn.cxx undname.cxx /Fewindebug\testundn.exe -nologo -Zi | tee -a testundn.log
echo Executing WinDebug\TestUndn.exe... | tee -a testundn.log
windebug\testundn | tee -a testundn.log
