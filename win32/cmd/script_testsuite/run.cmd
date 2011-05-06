::
:: PROJECT:     ReactOS CMD Testing Suite
:: LICENSE:     GPL v2 or any later version
:: FILE:        run.cmd
:: PURPOSE:     Runs the testing scripts
:: COPYRIGHT:   Copyright 2008 Colin Finck <mail@colinfinck.de>
::

@echo off
cls
echo ReactOS CMD Testing Suite
echo ==========================
echo.

:: Preparations
set failed_tests=0
set successful_tests=0
set test_count=0

if exist "temp\." (
	rmdir /s /q "temp"
)

mkdir "temp"

:: Run the tests
call :_runtest at
call :_runtest environment
call :_runtest if
call :_runtest redirect
call :_runtest set

:: Print the summary and clean up
echo Executed %test_count% tests, %successful_tests% successful, %failed_tests% failed
rmdir /s /q "temp"
goto :EOF

:: Functions
:_runtest
type "tests\%~1.cmd" > "temp\%~1.cmd"
type "lib\testlib.cmd" >> "temp\%~1.cmd"
call "temp\%~1.cmd"
goto :EOF
