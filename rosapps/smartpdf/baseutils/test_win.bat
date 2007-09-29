@echo off
pushd .
@call "C:\Program Files\Microsoft Visual Studio 8\Common7\Tools\vsvars32.bat"
IF ERRORLEVEL 1 goto VC_SETUP_FAILED

nmake -f Makefile.vc8
IF ERRORLEVEL 1 goto COMPILATION_FAILED

del *.obj *.ilk
test_file_util_d.exe
goto END

:VC_SETUP_FAILED
echo Failed to setup vc8
goto END

:COMPILATION_FAILED
echo Compilation failed!
goto END

:END
popd

