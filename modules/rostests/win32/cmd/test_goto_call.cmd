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

:: CALL does an extra round of variable substitution.

:: Existing labels (see below)
set VAR=TOTO
call :dest%VAR%
call :dest%%VAR%%
call :dest!VAR!
call :dest!!VAR!!

set VAR=1
call :dest%VAR%
call :dest%%VAR%%
call :dest!VAR!
call :dest!!VAR!!

:: Exercise different whitespace separations
call :dest1@space@@tab@ a b c
call @space@:dest1@space@@space@

:: Similar to GOTO, whitespace between ':' and the label name parameter are NOT ignored.
call :setError 0
call @space@:@space@dest1@space@@space@
if %errorlevel% equ 0 (echo Unexpected: CALL did not fail^^!) else echo OK

:: Only GOTO understands '+' instead of ':'.
:: Here '+dest1' is understood as a (non-existing) command.
call :setError 0
call @space@+dest1@space@@space@
if %errorlevel% equ 0 (echo Unexpected: CALL did not fail^^!) else echo OK


:: Calling a label with escape carets.
call :la^^bel1

:: Jumping to a label with escape carets.
goto :la^^bel2


:: Label with percents (should not be called)
:dest%%VAR%%
echo Unexpected CALL/GOTO jump^^!

:: Valid label (called from the variable substitution tests above)
:destTOTO
echo Hi there^^!
:: We exit this CALL invocation
goto :EOF

:: Valid label with arbitrary first character before ':'
:: (not just '@' as supposed by cmd_winetests!)
?:de^st1
echo goto with unrelated first character, and escape carets worked
echo Params: '%0', '%1', '%2', '%3'
:: We exit this CALL invocation
goto :EOF


:: Label with escape carets
:la^bel1
echo Unexpected CALL jump^^!
:la^^bel1
echo CALL with escape caret worked
:: We exit this CALL invocation
goto :EOF

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

:: This CALL will fail: :EOF is indeed a reserved label for GOTO only.
call :setError 0
call :EOF
if %errorlevel% equ 0 (echo Unexpected: CALL :EOF did not fail^^!) else echo OK

:: This CALL will succeed silently: only the first ':' of ::EOF is stripped,
:: thus calling in a new batch context GOTO :EOF.
call :setError 0
call ::EOF
if %errorlevel% neq 0 (echo Unexpected: CALL ::EOF did fail^^!) else echo OK

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
goto :continue

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
:: Next suite of tests.
::
:continue

echo --------- Testing CALL (triggers GOTO /?) ---------

::
:: Test that shows that CALL :label with the label name containing /?
:: at delayed time, internally calls GOTO, and will end up calling GOTO /?,
:: then jumps to the next command.
::
:: Adapted from https://stackoverflow.com/q/31987023/13530036
:: and from https://stackoverflow.com/a/38938416/13530036
:: The author of this test calls that "anonymous functions".
::
set "label=/?"

:: Test 1: GOTO /? will be called.
:: Since it is expected that the code below the CALL will also be called in
:: its context, but we want its output to be redirected to STDOUT, we use a
:: different redirection file so that it does not go into the tmp.txt file.
echo --- Direct label, redirection

:: Use an auxiliary CMD file
mkdir foobar && cd foobar

:: Call the label and redirect STDOUT to a file for later filtering.
call :%%label%% argument > tmp.txt
:: The following commands are called also within the CALL context.
:: The message will be displayed both in the CALL context, and in the main one.
echo Test message -- '%0','%*'>> tmpMsg.txt
:: Within the CALL context, this is the expected value of %0. Quit early.
if "%0" == ":/?" (
    echo Arguments: '%*'>> tmpMsg.txt
    exit /B
)

:: Back to the main context.
:: Display the redirected messages, filtering out the batch file name.
for /f "delims=" %%x in (tmpMsg.txt) do (
    set "msg=%%x"
    echo !msg:%0=!
)

:: Check whether CALL displayed GOTO help or CALL help.
:: We differentiate between both, because GOTO /? mentions CALL /?, but CALL
:: help does not mention GOTO, and contains the substring "CALL [" (part of the
:: syntax example) that allows to discriminate between both cases.
find "GOTO" tmp.txt > NUL && echo OK, GOTO help.|| echo Unexpected, no GOTO help^^!
find "CALL [" tmp.txt > NUL && echo Unexpected CALL help^^!

:: Cleanup
del tmp.txt tmpMsg.txt > NUL

:: Test 2: CALL /? will be called if piping.
echo --- Direct label, piping

call :%%label%% argument | (find "CALL [" > NUL && echo OK, CALL help.|| echo Unexpected, no CALL help^^!)
echo Test message -- '%0','%*'>> tmpMsg.txt
if "%0" == ":/?" (
    echo Arguments: '%*'>> tmpMsg.txt
    exit /B
)

:: Back to the main context.
:: Display the redirected messages, filtering out the batch file name.
for /f "delims=" %%x in (tmpMsg.txt) do (
    set "msg=%%x"
    echo !msg:%0=!
)

:: Cleanup
del tmpMsg.txt > NUL
cd .. & rd /s/q foobar


::
:: Repeat the same tests as above, but now with a slightly different label.
::
echo --------- Testing CALL with escape carets (triggers GOTO /?) ---------

set "help=^ /? ^^^^arg"

:: Test 1: GOTO /? will be called.
:: See the explanations in the previous tests above
echo --- Direct label, redirection

:: Use an auxiliary CMD file
mkdir foobar && cd foobar

:: Call the label and redirect STDOUT to a file for later filtering.
call :myLabel%%help%% ^^^^argument > tmp.txt
echo Test message -- '%0','%*'>> tmpMsg.txt
if "%0" == ":myLabel /?" (
    echo Arguments: '%*'>> tmpMsg.txt
    exit /B
)

:: Back to the main context.
:: Display the redirected messages, filtering out the batch file name.
for /f "delims=" %%x in (tmpMsg.txt) do (
    set "msg=%%x"
    echo !msg:%0=!
)

:: Check whether CALL displayed GOTO help or CALL help.
find "GOTO" tmp.txt > NUL && echo OK, GOTO help.|| echo Unexpected, no GOTO help^^!
find "CALL [" tmp.txt > NUL && echo Unexpected CALL help^^!

:: Cleanup
del tmp.txt tmpMsg.txt > NUL

:: Test 2: CALL /? will be called if piping.
echo --- Direct label, piping

call :myLabel%%help%% ^^^^argument | (find "CALL [" > NUL && echo OK, CALL help.|| echo Unexpected, no CALL help^^!)
echo Test message -- '%0','%*'>> tmpMsg.txt
if "%0" == ":myLabel /?" (
    echo Arguments: '%*'>> tmpMsg.txt
    exit /B
)

:: Back to the main context.
:: Display the redirected messages, filtering out the batch file name.
for /f "delims=" %%x in (tmpMsg.txt) do (
    set "msg=%%x"
    echo !msg:%0=!
)

:: Cleanup
del tmpMsg.txt > NUL
cd .. & rd /s/q foobar

goto :continue

:/?
:myLabel
echo Unexpected CALL or GOTO! Arguments: '%*'
exit /b 0


::
:: Next suite of tests.
::
:continue

::
:: This test will actually call the label.
:: Adapted from https://stackoverflow.com/a/38938416/13530036
::
echo --------- Testing CALL (NOT triggering GOTO /? or CALL /?) ---------

set "var=/?"
call :sub %%var%%
goto :otherTest

:sub
echo Arguments: '%*'
exit /b


:otherTest
call :sub2 "/?" Hello World
goto :continue

:sub2
:: 'args' contains the list of arguments
set "args=%*"
:: !args:%1=! will remove the first argument specified by %1 and keep the rest.
echo %~1 | (find "ECHO [" > NUL && echo OK, ECHO help.|| echo Unexpected, no ECHO help^^!)
echo/
echo !args:%1=!
exit /b


::
:: Next suite of tests.
::
:continue

::
:: CALL supports double delayed expansion.
:: Test from https://stackoverflow.com/a/31990563/13530036
::
echo --------- Testing CALL double delayed expansion ---------

set "label=myLabel"
set "pointer=^!label^!"
call :!pointer!
goto :finished

:myLabel
echo It works^^!
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
