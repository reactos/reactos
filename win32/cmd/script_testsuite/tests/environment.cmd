::
:: PROJECT:     ReactOS CMD Testing Suite
:: LICENSE:     GPL v2 or any later version
:: FILE:        tests/environment.cmd
:: PURPOSE:     Tests for the environment (like automatically set variables)
:: COPYRIGHT:   Copyright 2008 Colin Finck <mail@colinfinck.de>
::

:: ~0 contains the called path to the current script file
call :_testvar %~0 ~0

:: ~d0 contains the drive of the current script file
call :_testvar %~d0 ~d0

:: ~p0 contains the path of the current script file without the drive letter
call :_testvar %~p0 ~p0

:: ~dp0 contains the path to the current script file with the drive letter
call :_testvar %~dp0 ~dp0

:: ~dpf0 contains the full path to the current script file
call :_testvar %~dpf0 ~dpf0

:: ~n0 contains the name of the current script file without extension
call :_testvar %~n0 ~n0

:: ~x0 contains the extension of the current script file
call :_testvar %~x0 ~x0

:: ~a0 contains the attributes of the current script file
call :_testvar %~a0 ~a0

:: ~t0 contains the date and time of the current script file
call :_testvar "%~t0" ~t0

:: ~z0 contains the file size of the current script file in bytes
call :_testvar %~z0 ~z0

:: ~s0 and ~spf0 contain the short path to the current script file
call :_testvar %~s0 ~s0
call :_testvar %~s0 ~s0 %~spf0%

:: Now try to verify that the information is valid
set test_path=%~d0%~p0%~n0%~x0
call :_testvar %test_path% test_path %~dpf0

goto :EOF
