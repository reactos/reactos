::
:: PROJECT:     ReactOS CMD Testing Suite
:: LICENSE:     GPL v2 or any later version
:: FILE:        lib/testlib.cmd
:: PURPOSE:     Library with functions available for all tests
:: COPYRIGHT:   Copyright 2008 Colin Finck <mail@colinfinck.de>
::

:: Indicate that a test ran successfully
:_successful
set /a test_count+=1
set /a successful_tests+=1
goto :EOF


:: Indicate that a test failed
:: @param 1  Description of the test that failed
:_failed
set /a test_count+=1
set /a failed_tests+=1
echo Test "%~1" failed!
goto :EOF


:: Test whether a call succeeded
:: @param 1  The test command to run and check
:_test
%~1

if "%errorlevel%" == "0" (
	call :_successful
) else (
	call :_failed "%~1"
)
goto :EOF


:: Test whether a call failed
:: @param 1  The test command to run and check
:_testnot
%~1

if "%errorlevel%" == "0" (
	call :_failed "%~1"
) else (
	call :_successful
)
goto :EOF


:: Test the value of a variable
:: @param 1  The variable to check (like %test%)
:: @param 2  The variable name (like test)
:: @param 3  The expected result (like 5)
::           If this parameter wasn't given, _testvar checks if the variable is not ""
:_testvar
if "%~3" == "" (
	set testvar_operator=not
) else (
	set testvar_operator=
)

if %testvar_operator% "%~1" == "%~3" (
	call :_successful
) else (
	call :_failed "if %%~2%% == %~3, actual result was %~1"
)

goto :EOF
