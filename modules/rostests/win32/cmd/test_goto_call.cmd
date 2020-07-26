@echo off
setlocal enableextensions
setlocal enabledelayedexpansion


::
:: Tests for GOTO and CALL.
::


:: GOTO/CALL jump to labels present forward to their call-point. Only when
:: the label cannot be found forward, the search is then restarted from the
:: beginning of the batch file onwards up to the original call-point.

:: GOTO with a label parameter without ':' works.
goto test_start

:: Execution must never go there!
:test_goto
echo Unexpected GOTO jump^^!
exit
:test_call
echo Unexpected CALL jump^^!
goto :EOF


:test_start

:: Testing GOTO/CALL forwards.
echo --------- Testing GOTO ---------
goto :test_goto

:do_test_call
echo --------- Testing CALL within batch ---------
call :test_call
goto :continue

:test_goto
echo Test GOTO ok
:: GOTO also understands '+' instead of ':' in its label parameter.
goto +do_test_call

:test_call
echo Test CALL ok from %0
:: We exit this CALL invocation
goto :EOF


::
:: Next suite of tests.
::

:: GOTO label search algorithm ignores any whitespace between ':'
:: and the label name, as well as leading and trailing whitespace.
  :@tab@continue@space@@space@


:: Jumping to a label with escape carets.
goto :la^^bel2

:la^bel2
echo Unexpected GOTO jump^^!
:la^^bel2
echo GOTO with escape caret worked


:: Go to the next tests below.
goto :continue


::
:: Next suite of tests.
::
:continue


::
:: Extra GOTO syntax checks: separators in the label parameter
::

:: Whitespace
goto :testLbl1@tab@ignored
:testLbl1
echo Hi there^^!

:: Colon
goto :testLbl2:ignored
:testLbl2
echo Hi there^^!

:: Plus sign
goto :testLbl3+ignored
:testLbl3
echo Hi there^^!

:: Comma
goto :testLbl4,ignored
:testLbl4
echo Hi there^^!

:: Semicolon
goto :testLbl5;ignored
:testLbl5
echo Hi there^^!

:: Equals
goto :testLbl6;ignored
:testLbl6
echo Hi there^^!


::
:: Testing :EOF support
::
echo --------- Testing :EOF support ---------

:: Use an auxiliary CMD file to test GOTO :EOF
mkdir foobar && cd foobar

:: GOTO :EOF is available only if commands extensions are enabled
echo @echo off> tmp.cmd
echo setlocal disableextensions>> tmp.cmd
echo goto :eof>> tmp.cmd
call :setError 0
cmd /c tmp.cmd
if %errorlevel% equ 0 (echo Unexpected: GOTO :EOF did not fail^^!) else echo OK

:: GOTO :EOF is done only if the ":EOF" part is followed by whitespace or ends.
:: The following two GOTO's fail because the labels cannot be found.
echo @echo off> tmp.cmd
echo setlocal enableextensions>> tmp.cmd
echo goto :eof,lol>> tmp.cmd
echo echo Batch continues^^!>> tmp.cmd
call :setError 0
cmd /c tmp.cmd
if %errorlevel% equ 0 (echo Unexpected: GOTO :eof,lol did not fail^^!) else echo OK

echo @echo off> tmp.cmd
echo setlocal enableextensions>> tmp.cmd
echo goto :eof:lol>> tmp.cmd
echo echo Batch continues^^!>> tmp.cmd
call :setError 0
cmd /c tmp.cmd
if %errorlevel% equ 0 (echo Unexpected: GOTO :eof:lol did not fail^^!) else echo OK

:: GOTO :EOF expects at least one whitespace character before anything else.
:: Not even '+',':' or other separators are allowed.
echo @echo off> tmp.cmd
echo setlocal enableextensions>> tmp.cmd
echo goto :eof+lol>> tmp.cmd
echo echo Batch continues^^!>> tmp.cmd
call :setError 0
cmd /c tmp.cmd
if %errorlevel% equ 0 (echo Unexpected: GOTO :eof+lol did not fail^^!) else echo OK

:: This GOTO :EOF works.
echo @echo off> tmp.cmd
echo setlocal enableextensions>> tmp.cmd
echo goto :eof@tab@+lol>> tmp.cmd
echo echo You should not see this^^!>> tmp.cmd
call :setError 0
cmd /c tmp.cmd
if %errorlevel% neq 0 (echo Unexpected: GOTO :EOF did fail^^!) else echo OK


:: Cleanup
cd .. & rd /s/q foobar


::
:: Testing GOTO/CALL from and to within parenthesized blocks.
::

echo --------- Testing GOTO within block ---------
(echo Block-test 1: Single-line& goto :block2 & echo Unexpected Block-test 1^^!)
echo Unexpected echo 1^^!

:block2
(
echo Block-test 2: Multi-line
goto :block3
echo Unexpected Block-test 2^^!
)
echo Unexpected echo 2-3^^!

:test_call_block
echo Test CALL in block OK from %0
:: We exit this CALL invocation
goto :EOF

(
:block3
echo --------- Testing CALL within block ---------
echo Block-test 3: CALL in block
call :test_call_block
echo CALL done
)

goto :block4
echo Unexpected echo 4^^!
(
:block4
echo Block-test 4 OK
)


::
:: Testing GOTO/CALL from within FOR and IF.
:: This is a situation similar to the parenthesized blocks.
:: See bug-report CORE-13713
::

:: Testing CALL within FOR
echo --------- Testing CALL within FOR ---------
for /L %%A IN (0,1,3) DO (
    set Number=%%A
    if %%A==2 call :out_of_loop_1 %%A
    if %%A==2 (echo %%A IS equal to 2) else (echo %%A IS NOT equal to 2)
)
goto :continue_2
:out_of_loop_1
echo Out of FOR 1 CALL from %0, number is %1
:: We exit this CALL invocation
goto :EOF
:continue_2


:: Testing GOTO within FOR
echo --------- Testing GOTO within FOR ---------
for /L %%A IN (0,1,3) DO (
    set Number=%%A
    if %%A==2 goto :out_of_loop_2
    echo %%A IS NOT equal to 2
)
echo Unexpected FOR echo 2^^!
:out_of_loop_2
echo Out of FOR 2, number is %Number%



::
:: Show how each different FOR-loop stops when a GOTO is encountered.
::
echo --------- Testing FOR loop stopping with GOTO ---------

:: FOR - Stops directly
echo --- FOR
@echo on
for %%A in (1,2,3,4,5,6,7,8,9,10) do (
    set Number=%%A
    if %%A==5 goto :out_of_loop_2a
)
echo Unexpected FOR echo 2a^^!
:out_of_loop_2a
echo Out of FOR 2a, number is %Number%
@echo off


:: FOR /R - Stops directly
echo --- FOR /R

:: Use auxiliary directoreis to test for /R
mkdir foobar && cd foobar
mkdir foo1
mkdir foo2
mkdir bar1

@echo on
for /r %%A in (1,2,3,4,5,6,7,8,9,10) do (
    set Number=%%~nA
    if %%~nA==5 goto :out_of_loop_2b
)
echo Unexpected FOR echo 2b^^!
:out_of_loop_2b
echo Out of FOR 2b, number is %Number%
@echo off

:: Cleanup
cd .. & rd /s/q foobar


:: FOR /L - Does not stop directly. It continues looping until the end
:: but does not execute its body code. This can cause problems e.g. for
:: infinite loops "for /l %a in () do ( ... )" that are exited by EXIT /B,
:: since the body code stops being executed, but the loop itself continues
:: running forever.
echo --- FOR /L
@echo on
for /l %%A in (1,1,10) do (
    set Number=%%A
    if %%A==5 goto :out_of_loop_2c
)
echo Unexpected FOR echo 2c^^!
:out_of_loop_2c
echo Out of FOR 2c, number is %Number%
@echo off


:: FOR /F - Stops directly.
echo --- FOR /F
@echo on
for %%T in ( "1:2:3" "4:5:6:7" "8:9:10" ) do (
   set "pc=%%~T"
   for /f "delims=" %%A in (^"!pc::^=^
% New line %
!^") do (

    set Number=%%A
    if %%A==5 goto :out_of_loop_2d
)
)
echo Unexpected FOR echo 2d^^!
:out_of_loop_2d
echo Out of FOR 2d, number is %Number%
@echo off



:: Testing CALL within IF
echo --------- Testing CALL within IF ---------
if 1==1 (
    call :out_of_if_1 123
    echo Success IF echo 1
)
goto :continue_3
:out_of_if_1
echo Out of IF CALL from %0, number is %1
:: We exit this CALL invocation
goto :EOF
:continue_3


:: Testing GOTO within IF
echo --------- Testing GOTO within IF ---------
if 1==1 (
    goto :out_of_if_2
    echo Unexpected IF echo 2a^^!
)
echo Unexpected IF echo 2b^^!
:out_of_if_2
echo Out of IF ok

:: Same, but with line-continuation at the closing parenthesis of the IF block.
if 1==1 (
:labelA
    echo A
) ^
else (
:labelB
    echo B
    goto :continue
)
:: We are jumping inside the IF, whose block will be interpreted as
:: separate commands; thus we will also run the :labelB block as well.
goto :labelA


::
:: Next suite of tests.
::
:continue

:: Testing EXIT within IF
echo --------- Testing EXIT within IF ---------

:: Use a CALL context, and we will only check EXIT /B.
call :doExitIfTest 1
call :doExitIfTest 2
goto :finished

:doExitIfTest
if %1==1 (
    echo First block
    exit /b
    echo Unexpected first block^^!
) else (
    echo Second block
    exit /b
    echo Unexpected second block^^!
)
echo You won't see this^^!
exit /b



::
:: Finished!
::
:finished
echo --------- Finished  --------------
goto :EOF

:: Subroutine to set errorlevel and return
:: in windows nt 4.0, this always sets errorlevel 1, since /b isn't supported
:setError
exit /B %1
:: This line runs under cmd in windows NT 4, but not in more modern versions.
