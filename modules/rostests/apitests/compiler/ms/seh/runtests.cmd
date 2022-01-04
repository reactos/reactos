REM This is a simple example script to verify that the tests are working on 
REM windows. This script will be superceded by the lit/lnt as they are
REM migrated to the LLVM test-suite.   

REM set FLAGS to the desired optimization level
set FLAGS=/Od

cl %FLAGS% seh0001.c
seh0001.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0001.exe
cl %FLAGS% seh0002.c
seh0002.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0002.exe
cl %FLAGS% seh0003.c
seh0003.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0003.exe
cl %FLAGS% seh0004.c
seh0004.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0004.exe
cl %FLAGS% seh0005.c
seh0005.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0005.exe
cl %FLAGS% seh0006.c
seh0006.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0006.exe
cl %FLAGS% seh0007.c
seh0007.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0007.exe
cl %FLAGS% seh0008.c
seh0008.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0008.exe
cl %FLAGS% seh0009.c
seh0009.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0009.exe
cl %FLAGS% seh0010.c
seh0010.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0010.exe
cl %FLAGS% seh0011.c
seh0011.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0011.exe
cl %FLAGS% seh0012.c
seh0012.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0012.exe
cl %FLAGS% seh0013.c
seh0013.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0013.exe
cl %FLAGS% seh0014.c
seh0014.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0014.exe
cl %FLAGS% seh0015.c
seh0015.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0015.exe
cl %FLAGS% seh0016.c
seh0016.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0016.exe
cl %FLAGS% seh0017.c
seh0017.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0017.exe
cl %FLAGS% seh0018.c
seh0018.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0018.exe
cl %FLAGS% seh0019.c
seh0019.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0019.exe
cl %FLAGS% seh0020.c
seh0020.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0020.exe
cl %FLAGS% seh0021.c
seh0021.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0021.exe
cl %FLAGS% seh0022.c
seh0022.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0022.exe
cl %FLAGS% seh0023.c
seh0023.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0023.exe
cl %FLAGS% seh0024.c
seh0024.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0024.exe
cl %FLAGS% seh0025.c
seh0025.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0025.exe
cl %FLAGS% seh0026.c
seh0026.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0026.exe
cl %FLAGS% seh0027.c
seh0027.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0027.exe
cl %FLAGS% seh0028.c
seh0028.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0028.exe
cl %FLAGS% seh0029.c
seh0029.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0029.exe
cl %FLAGS% seh0030.c
seh0030.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0030.exe
cl %FLAGS% seh0031.c
seh0031.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0031.exe
cl %FLAGS% seh0032.c
seh0032.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0032.exe
cl %FLAGS% seh0033.c
seh0033.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0033.exe
cl %FLAGS% seh0034.c
seh0034.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0034.exe
cl %FLAGS% seh0035.c
seh0035.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0035.exe
cl %FLAGS% seh0036.c
seh0036.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0036.exe
cl %FLAGS% seh0037.c
seh0037.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0037.exe
cl %FLAGS% seh0038.c
seh0038.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0038.exe
cl %FLAGS% seh0039.c
seh0039.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0039.exe
cl %FLAGS% seh0040.c
seh0040.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0040.exe
cl %FLAGS% seh0041.c
seh0041.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0041.exe
cl %FLAGS% seh0042.c
seh0042.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0042.exe
cl %FLAGS% seh0043.c
seh0043.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0043.exe
cl %FLAGS% seh0044.c
seh0044.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0044.exe
cl %FLAGS% seh0045.c
seh0045.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0045.exe
cl %FLAGS% seh0046.c
seh0046.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0046.exe
cl %FLAGS% seh0047.c
seh0047.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0047.exe
cl %FLAGS% seh0048.c
seh0048.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0048.exe
cl %FLAGS% seh0049.c
seh0049.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0049.exe
cl %FLAGS% seh0050.c
seh0050.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0050.exe
cl %FLAGS% seh0051.c
seh0051.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0051.exe
cl %FLAGS% seh0052.c
seh0052.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0052.exe
cl %FLAGS% seh0053.c
seh0053.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0053.exe
cl %FLAGS% seh0054.c
seh0054.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0054.exe
cl %FLAGS% seh0055.c
seh0055.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0055.exe
cl %FLAGS% seh0056.c
seh0056.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0056.exe
cl %FLAGS% seh0057.c
seh0057.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0057.exe
cl %FLAGS% seh0058.c
seh0058.exe
@if NOT ERRORLEVEL 0 echo FAIL seh0058.exe
cl %FLAGS% -c seh_noreturn.c
REM seh_noreturn.c is a compile only test.
@if NOT ERRORLEVEL 0 echo FAIL seh_noreturn.exe

cl %FLAGS% xcpt4u.c
xcpt4u.exe > xcpt4u.test.out
@if NOT ERRORLEVEL 0 echo FAIL xcpt4u.exe
diff xcpt4u.test.out xcpt4u.correct
@if ERRORLEVEL 1 echo FAIL xcpt4u.exe with diffs!

cl %FLAGS% sehframes.cpp
sehframes.exe
@if NOT ERRORLEVEL 0 echo FAIL sehframes.exe
diff sehframes.test.out sehframes.out
@if ERRORLEVEL 1 echo FAIL sehframes.exe with diffs!