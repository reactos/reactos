::
:: PROJECT:     ReactOS CMD Testing Suite
:: LICENSE:     GPL v2 or any later version
:: FILE:        tests/set.cmd
:: PURPOSE:     Tests for the "set" command
:: COPYRIGHT:   Copyright 2005 Royce Mitchell III
::              Copyright 2008 Colin Finck <mail@colinfinck.de>
::

:: Test the /A parameter
call :_test "set /a a=1"
call :_testvar %a% a 1

call :_test "set /a b=a"
call :_testvar %b% b 1

call :_test "set /a a=!5"
call :_testvar %a% a 0

call :_test "set /a a=!a"
call :_testvar %a% a 1

call :_test "set /a a=~5"
call :_testvar %a% a -6

call :_test "set /a a=5,a=-a"
call :_testvar %a% a -5

call :_test "set /a a=5*7"
call :_testvar %a% a 35

call :_test "set /a a=2000/10"
call :_testvar %a% a 200

call :_test "set /a a=42%%%%9"
call :_testvar %a% a 6

call :_test "set /a a=5%%2"
call :_testvar %a% a 5

call :_test "set /a a=42%13"
call :_testvar %a% a 423

call :_test "set /a a=7+9"
call :_testvar %a% a 16

call :_test "set /a a=9-7"
call :_testvar %a% a 2

set /a a=9^<^<2
call :_testvar %a% a 36

set /a a=36^>^>2
call :_testvar %a% a 9

set /a a=42^&9
call :_testvar %a% a 8

set /a a=32^9
call :_testvar %a% a 329

set /a a=32^^9
call :_testvar %a% a 41

set /a a=10^|22
call :_testvar %a% a 30

call :_test "set /a a=2,a*=3"
call :_testvar %a% a 6

call :_test "set /a a=11,a/=2"
call :_testvar %a% a 5

call :_test "set /a a=42,a%%%%=9"
call :_testvar %a% a 6

call :_test "set /a a=7,a+=9"
call :_testvar %a% a 16

call :_test "set /a a=9,a-=7"
call :_testvar %a% a 2

set /a a=42,a^&=9
call :_testvar %a% a 8

set /a a=32,a^^=9
call :_testvar %a% a 41

set /a a=10,a^|=22
call :_testvar %a% a 30

set /a a=9,a^<^<=2
call :_testvar %a% a 36

set /a a=36,a^>^>=2
call :_testvar %a% a 9

call :_test "set /a a=1,2"
call :_testvar %a% a 1

call :_test "set /a a=(a=1,a+2)"
call :_testvar %a% a 3

goto :EOF
