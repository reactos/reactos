::
:: PROJECT:     ReactOS CMD Testing Suite
:: LICENSE:     GPL v2 or any later version
:: FILE:        tests/if.cmd
:: PURPOSE:     Tests for the "if" command
:: COPYRIGHT:   Copyright 2005 Royce Mitchell III
::              Copyright 2008 Colin Finck <mail@colinfinck.de>
::

:: Bugs in the if code
if not "=="=="==" call :_failed "if not "=="=="==""
if "=="=="==" call :_successful

if "1"=="2" (
	call :_failed "if "1"=="2""
) else (
	call :_successful
)

if not "1"=="1" (
	call :_failed "if "1"=="1""
) else (
	call :_successful
)

goto :EOF
