@echo off

echo ------------ Testing FOR loop ------------
echo --- Multiple lines
for %%i in (A
B
C) do echo %%i

echo --- Lines and spaces
for %%i in (D
 E
 F) do echo %%i

echo --- Multiple lines and commas
for %%i in (G,
H,
I
) do echo %%i

echo --- Multiple lines and %%I
:: The FOR-variable is case-sensitive
for %%i in (J
 K
 L) do echo %%I

echo --- Multiple lines and %%j
for %%i in (M,
N,
O
) do echo %%j


echo ---------- Testing AND operator ----------
:: Test for TRUE condition - Should be displayed
ver | find "Ver" > NUL && echo TRUE AND condition

:: Test for FALSE condition - Should not display
ver | find "1234" > NUL && echo FALSE AND condition

echo ---------- Testing OR operator -----------
:: Test for TRUE condition - Should not display
ver | find "Ver" > NUL || echo TRUE OR condition

:: Test for FALSE condition - Should be displayed
ver | find "1234" > NUL || echo FALSE OR condition


echo ------------- End of Testing -------------
