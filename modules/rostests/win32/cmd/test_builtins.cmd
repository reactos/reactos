@echo off

::
:: Some basic tests
::

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



::
:: Testing CMD exit codes and errorlevels.
::
:: Observations:
:: - OR operator || converts the LHS error code to ERRORLEVEL only on failure;
:: - Pipe operator | converts the last error code to ERRORLEVEL.
::
:: See https://stackoverflow.com/a/34987886/13530036
:: and https://stackoverflow.com/a/34937706/13530036
:: for more details.
::
setlocal enableextensions

echo ---------- Testing CMD exit codes and errorlevels ----------

:: Tests for CMD returned exit code.

echo --- CMD /C Direct EXIT call

call :setError 0
cmd /c "exit 42"
call :checkErrorLevel 42

call :setError 111
cmd /c "exit 42"
call :checkErrorLevel 42

echo --- CMD /C Direct EXIT /B call

call :setError 0
cmd /c "exit /b 42"
call :checkErrorLevel 42

call :setError 111
cmd /c "exit /b 42"
call :checkErrorLevel 42

:: Non-existing ccommand, or command that only changes
:: the returned code (but NOT the ERRORLEVEL) and EXIT.

echo --- CMD /C Non-existing command

:: EXIT alone does not change the ERRORLEVEL
call :setError 0
cmd /c "nonexisting & exit"
call :checkErrorLevel 9009

call :setError 111
cmd /c "nonexisting & exit"
call :checkErrorLevel 9009

call :setError 0
cmd /c "nonexisting & exit /b"
call :checkErrorLevel 9009

call :setError 111
cmd /c "nonexisting & exit /b"
call :checkErrorLevel 9009

echo --- CMD /C RMDIR (no ERRORLEVEL set)

call :setError 0
cmd /c "rmdir nonexisting & exit"
call :checkErrorLevel 0

call :setError 111
cmd /c "rmdir nonexisting & exit"
call :checkErrorLevel 0

call :setError 0
cmd /c "rmdir nonexisting & exit /b"
call :checkErrorLevel 0

call :setError 111
cmd /c "rmdir nonexisting & exit /b"
call :checkErrorLevel 0

:: Failing command (sets ERRORLEVEL to 1) and EXIT
echo --- CMD /C DIR (sets ERRORLEVEL) - With failure

:: EXIT alone does not change the ERRORLEVEL
call :setError 0
cmd /c "dir nonexisting>NUL & exit"
call :checkErrorLevel 1

call :setError 111
cmd /c "dir nonexisting>NUL & exit"
call :checkErrorLevel 1

call :setError 0
cmd /c "dir nonexisting>NUL & exit /b"
call :checkErrorLevel 1

call :setError 111
cmd /c "dir nonexisting>NUL & exit /b"
call :checkErrorLevel 1

:: Here EXIT changes the ERRORLEVEL
call :setError 0
cmd /c "dir nonexisting>NUL & exit 42"
call :checkErrorLevel 42

call :setError 111
cmd /c "dir nonexisting>NUL & exit 42"
call :checkErrorLevel 42

call :setError 0
cmd /c "dir nonexisting>NUL & exit /b 42"
call :checkErrorLevel 42

call :setError 111
cmd /c "dir nonexisting>NUL & exit /b 42"
call :checkErrorLevel 42

:: Succeeding command (sets ERRORLEVEL to 0) and EXIT
echo --- CMD /C DIR (sets ERRORLEVEL) - With success

call :setError 0
cmd /c "dir>NUL & exit"
call :checkErrorLevel 0

call :setError 111
cmd /c "dir>NUL & exit"
call :checkErrorLevel 0

call :setError 0
cmd /c "dir>NUL & exit 42"
call :checkErrorLevel 42

call :setError 111
cmd /c "dir>NUL & exit 42"
call :checkErrorLevel 42

call :setError 0
cmd /c "dir>NUL & exit /b 42"
call :checkErrorLevel 42

call :setError 111
cmd /c "dir>NUL & exit /b 42"
call :checkErrorLevel 42


:: Same sorts of tests, but now from within an external batch file:
:: Tests for CALL command returned exit code.

:: Use an auxiliary CMD file
mkdir foobar && cd foobar

:: Non-existing ccommand, or command that only changes
:: the returned code (but NOT the ERRORLEVEL) and EXIT.

echo --- CALL Batch Non-existing command

:: EXIT alone does not change the ERRORLEVEL
echo nonexisting ^& exit /b> tmp.cmd
call :setError 0
call tmp.cmd
call :checkErrorLevel 9009

echo nonexisting ^& exit /b> tmp.cmd
call :setError 111
call tmp.cmd
call :checkErrorLevel 9009

:: These tests show that || converts the returned error code
:: from RMDIR on failure, and converts it to an ERRORLEVEL
:: (first two tests: no ||, thus no ERRORLEVEL set;
:: last two tests: ||used and ERRORLEVEL is set).
::

echo --- CALL Batch RMDIR (no ERRORLEVEL set)

:: This test shows that if a batch returns error code 0 from CALL,
:: then CALL will keep the existing ERRORLEVEL (here, 111)...
echo rmdir nonexisting> tmp.cmd
echo exit /b>> tmp.cmd
call :setError 0
call tmp.cmd
call :checkErrorLevel 0

echo rmdir nonexisting> tmp.cmd
echo exit /b>> tmp.cmd
call :setError 111
call tmp.cmd
call :checkErrorLevel 111

echo --- CALL Batch RMDIR with ^|^| (sets ERRORLEVEL)

:: ... but if a non-zero error code is returned from CALL,
:: then CALL uses it as the new ERRORLEVEL.
echo rmdir nonexisting ^|^| rem> tmp.cmd
echo exit /b>> tmp.cmd
call :setError 0
call tmp.cmd
call :checkErrorLevel 2
:: This gives the same effect, since the last command's error code
:: is returned and transformed by CALL into an ERRORLEVEL:
echo rmdir nonexisting> tmp.cmd
call :setError 0
call tmp.cmd
call :checkErrorLevel 2

echo rmdir nonexisting ^|^| rem> tmp.cmd
echo exit /b>> tmp.cmd
call :setError 111
call tmp.cmd
call :checkErrorLevel 2
:: This gives the same effect, since the last command's error code
:: is returned and transformed by CALL into an ERRORLEVEL:
echo rmdir nonexisting> tmp.cmd
call :setError 111
call tmp.cmd
call :checkErrorLevel 2


:: Failing command (sets ERRORLEVEL to 1) and EXIT
echo --- CALL Batch DIR (sets ERRORLEVEL) - With failure

echo dir nonexisting^>NUL> tmp.cmd
call :setError 0
call tmp.cmd
call :checkErrorLevel 1

echo dir nonexisting^>NUL> tmp.cmd
call :setError 111
call tmp.cmd
call :checkErrorLevel 1

echo dir nonexisting^>NUL ^& goto :eof> tmp.cmd
call :setError 0
call tmp.cmd
call :checkErrorLevel 1

echo dir nonexisting^>NUL ^& goto :eof> tmp.cmd
call :setError 111
call tmp.cmd
call :checkErrorLevel 1

echo dir nonexisting^>NUL ^& exit /b> tmp.cmd
call :setError 0
call tmp.cmd
call :checkErrorLevel 1

echo dir nonexisting^>NUL ^& exit /b> tmp.cmd
call :setError 111
call tmp.cmd
call :checkErrorLevel 1

echo dir nonexisting^>NUL ^& exit /b 42 > tmp.cmd
call :setError 0
call tmp.cmd
call :checkErrorLevel 42

echo dir nonexisting^>NUL ^& exit /b 42 > tmp.cmd
call :setError 111
call tmp.cmd
call :checkErrorLevel 42

:: Succeeding command (sets ERRORLEVEL to 0) and EXIT
echo --- CALL Batch DIR (sets ERRORLEVEL) - With success

echo dir^>NUL> tmp.cmd
call :setError 0
call tmp.cmd
call :checkErrorLevel 0

echo dir^>NUL> tmp.cmd
call :setError 111
call tmp.cmd
call :checkErrorLevel 0

echo dir^>NUL ^& goto :eof> tmp.cmd
call :setError 0
call tmp.cmd
call :checkErrorLevel 0

echo dir^>NUL ^& goto :eof> tmp.cmd
call :setError 111
call tmp.cmd
call :checkErrorLevel 0

echo dir^>NUL ^& exit /b> tmp.cmd
call :setError 0
call tmp.cmd
call :checkErrorLevel 0

echo dir^>NUL ^& exit /b> tmp.cmd
call :setError 111
call tmp.cmd
call :checkErrorLevel 0

echo dir^>NUL ^& exit /b 42 > tmp.cmd
call :setError 0
call tmp.cmd
call :checkErrorLevel 42

echo dir^>NUL ^& exit /b 42 > tmp.cmd
call :setError 111
call tmp.cmd
call :checkErrorLevel 42


:: Cleanup
del tmp.cmd
cd .. & rmdir /s/q foobar


::
:: Finished!
::
echo --------- Finished  --------------
goto :EOF

:checkErrorLevel
if %errorlevel% neq %1 (echo Unexpected errorlevel %errorlevel%, expected %1) else echo OK
goto :eof

:: Subroutine to set errorlevel and return
:: in windows nt 4.0, this always sets errorlevel 1, since /b isn't supported
:setError
exit /B %1
:: This line runs under cmd in windows NT 4, but not in more modern versions.
