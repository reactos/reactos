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

echo --- FOR /F token parsing
:: This test requires extensions being enabled
setlocal enableextensions

set TEST_STRING="_ ` a b c d e f g h i j k l m n o p q r s t u v w x y z { | } ~ ? @ [ \ ] _ ` a b c d e f g h i j k l m n o p q r s t u v w x y z { | } ~ ? @ [ \ ]"
set "ECHO_STRING=?=%%? @=%%@ A=%%A B=%%B C=%%C D=%%D E=%%E F=%%F G=%%G H=%%H I=%%I J=%%J K=%%K L=%%L M=%%M N=%%N O=%%O P=%%P Q=%%Q R=%%R S=%%S T=%%T U=%%U V=%%V W=%%W X=%%X Y=%%Y Z=%%Z [=%%[ \=%%\ ]=%%] ^^=%%^^ _=%%_ `=%%` a=%%a b=%%b c=%%c d=%%d e=%%e f=%%f g=%%g h=%%h i=%%i j=%%j k=%%k l=%%l m=%%m n=%%n o=%%o p=%%p q=%%q r=%%r s=%%s t=%%t u=%%u v=%%v w=%%w x=%%x y=%%y z=%%z {=%%{ ^|=%%^| }=%%} ^~=%%^~"

echo.

:: Bug 1: Ranges that are not specified in increasing order are ignored.
:: Token numbers strictly greater than 31 are just ignored, and if they
:: appear in a range, the whole range is ignored.

for /f "tokens=30-32" %%? in (%TEST_STRING%) do echo %ECHO_STRING%
echo.
for /f "tokens=5-1" %%? in (%TEST_STRING%) do echo %ECHO_STRING%
echo.

:: Bug 2: Ranges that partially overlap: too many variables are being allocated,
:: while only a subset is actually used. This leads to the extra variables returning
:: empty strings.

for /f "tokens=1-31,31,31" %%? in (%TEST_STRING%) do echo %ECHO_STRING%
echo.
for /f "tokens=1-31,1-31" %%? in (%TEST_STRING%) do echo %ECHO_STRING%
echo.
for /f "tokens=1-5,3,5,6" %%? in (%TEST_STRING%) do echo %ECHO_STRING%
echo.
for /f "tokens=1-31,* tokens=1-31 tokens=1-20,*" %%? in (%TEST_STRING%) do echo %ECHO_STRING%
echo.

:: For comparison, this works:
for /f "tokens=1-5,6" %%? in (%TEST_STRING%) do echo %ECHO_STRING%
echo.
for /f "tokens=1-5,6-10" %%? in (%TEST_STRING%) do echo %ECHO_STRING%
echo.

endlocal


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
:: ERRORLEVEL tests for special commands.
::
:: For some commands, the errorlevel is set differently,
:: whether or not they are run within a .BAT, a .CMD, or
:: directly from the command-line.
::
:: These commands are:
:: APPEND/DPATH, ASSOC, FTYPE, PATH, PROMPT, SET.
::
:: See https://ss64.com/nt/errorlevel.html for more details.
::

echo ---------- Testing ERRORLEVEL in .BAT and .CMD ----------

:: Use an auxiliary CMD file
mkdir foobar && cd foobar

echo --- In .BAT file
call :outputErrLvlTestToBatch tmp.bat
cmd /c tmp.bat
del tmp.bat

echo --- In .CMD file
call :outputErrLvlTestToBatch tmp.cmd
cmd /c tmp.cmd
del tmp.cmd

:: Cleanup
cd .. & rmdir /s/q foobar

:: Go to the next tests below.
goto :continue

:: ERRORLEVEL test helper function
:outputErrLvlTestToBatch (filename)

echo @echo off> %1
echo setlocal enableextensions>> %1

:: Reset the errorlevel
echo call :zeroErrLvl>> %1

:: dpath
:: echo %errorlevel%
:: dpath xxx
:: echo %errorlevel%
:: dpath
:: echo %errorlevel%

echo assoc^>NUL>> %1
echo echo %%errorlevel%%>> %1
echo assoc .nonexisting^>NUL>> %1
echo echo %%errorlevel%%>> %1
echo assoc .nonexisting^=^>NUL>> %1
echo echo %%errorlevel%%>> %1

:: ftype
:: echo %errorlevel%
:: ftype xxx
:: echo %errorlevel%
:: ftype
:: echo %errorlevel%

echo path^>NUL>> %1
echo echo %%errorlevel%%>> %1
echo path^;^>NUL>> %1
echo echo %%errorlevel%%>> %1

echo prompt^>NUL>> %1
echo echo %%errorlevel%%>> %1
echo prompt ^$p^$g^>NUL>> %1
echo echo %%errorlevel%%>> %1
echo prompt foobar^>NUL>> %1
echo echo %%errorlevel%%>> %1

echo set^>NUL>> %1
echo echo %%errorlevel%%>> %1
echo set nonexisting^>NUL>> %1
echo echo %%errorlevel%%>> %1
echo set nonexisting^=^>NUL>> %1
echo echo %%errorlevel%%>> %1
echo set nonexisting^=trololol^>NUL>> %1
echo echo %%errorlevel%%>> %1
echo set nonexisting^=^>NUL>> %1
echo echo %%errorlevel%%>> %1

echo goto :eof>> %1

:: Zero ERRORLEVEL
echo :zeroErrLvl>> %1
echo exit /B 0 >> %1

goto :eof


::
:: Next suite of tests.
::
:continue


:: Testing different ERRORLEVELs from the SET command.
:: See https://ss64.com/nt/set.html for more details.

echo ---------- Testing SET /A ERRORLEVELs ----------

echo --- Success
call :setError 0
set /a "total=1+1"
call :checkErrorLevel 0
echo %errorlevel%
echo %total%

echo --- Unbalanced parentheses
call :setError 0
set /a "total=(2+1"
call :checkErrorLevel 1073750988
echo %errorlevel%
echo %total%

echo --- Missing operand
call :setError 0
set /a "total=5*"
call :checkErrorLevel 1073750989
echo %errorlevel%
echo %total%

echo --- Syntax error
call :setError 0
set /a "total=7$3"
call :checkErrorLevel 1073750990
echo %errorlevel%
echo %total%

echo --- Invalid number
call :setError 0
set /a "total=0xdeadbeeg"
call :checkErrorLevel 1073750991
echo %errorlevel%
echo %total%

echo --- Number larger than 32-bits
call :setError 0
set /a "total=999999999999999999999999"
call :checkErrorLevel 1073750992
echo %errorlevel%
echo %total%

echo --- Division by zero
call :setError 0
set /a "total=1/0"
call :checkErrorLevel 1073750993
echo %errorlevel%
echo %total%



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
