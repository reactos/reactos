::
:: PROJECT:     ReactOS CMD Testing Suite
:: LICENSE:     GPL v2 or any later version
:: FILE:        tests/redirect.cmd
:: PURPOSE:     Tests for redirections
:: COPYRIGHT:   Copyright 2008 Colin Finck <mail@colinfinck.de>
::

:: One redirect, the file must exist
call :_test "echo moo > temp\redirect_temp.txt"
call :_test "type temp\redirect_temp.txt >nul"
call :_test "find "moo" temp\redirect_temp.txt >nul"

:: Add the string yet another time to the file
call :_test "echo moo >> temp\redirect_temp.txt"

set moo_temp=0

:: Count the moo's in the file (must be 2 now)
:: No idea why so many percent signs are necessary here :-)
call :_test "for /f "usebackq" %%%%i in (`findstr moo temp\redirect_temp.txt`) do set /a moo_temp+=1"
call :_testvar %moo_temp% moo_temp 2

:: Two redirects, the file in the middle mustn't exist after this call
call :_test "echo moo > temp\redirect_temp2.txt > nul"
call :_testnot "type temp\redirect_temp2.txt 2>nul"

goto :EOF
