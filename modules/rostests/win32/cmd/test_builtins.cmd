@echo off
echo ------------ Testing for1 ------------
echo --- plain FOR with multiple lines
for %%i in (A
B
C) do echo %%i
echo ------------ Testing for2 ------------
echo --- plain FOR with lines and spaces
for %%i in (D
 E
 F) do echo %%i
echo ------------ Testing for3 ------------
echo --- plain FOR with multiple lines and commas
for %%i in (G,
H,
I
) do echo %%i
echo ------------ Testing for4 ------------
echo --- plain FOR with multiple lines and %%I
for %%i in (J
 K
 L) do echo %%I
echo ------------ Testing for5 ------------
echo --- plain FOR with multiple lines and %%j
for %%i in (M,
N,
O
) do echo %%j
echo ------------ Testing Amp-Amp ---------
echo --- test for something that is TRUE
ver | find "Ver" > NUL && Echo TRUE Amp-Amp
echo ------------ Testing Amp-Amp ---------
echo --- test for something that is FALSE
ver | find "1234" > NUL && Echo FALSE Amp-Amp
echo ------------ Testing Pipe-Pipe -------
echo --- test for something that is TRUE
ver | find "Ver" > NUL || Echo TRUE Pipe-Pipe
echo ------------ Testing Pipe-Pipe -------
echo --- test for something that is FALSE
ver | find "1234" > NUL || Echo FALSE Pipe-Pipe
echo ------------ End of Testing ------------
echo --- Testing ends here
